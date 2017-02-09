"""Implements the main DSE loop in spark."""
import maxbuild

import argparse
import itertools
import json
import os
import pprint
import re
import shutil
import subprocess
import sys
import pandas as pd
from tabulate import tabulate
from html import HTML
from bs4 import BeautifulSoup

from os import listdir
from os.path import isfile, join
from scipy import io, sparse
from subprocess import call
from termcolor import colored

PRJ = 'Spmv'

TARGET_DFE_MOCK = 'dfe_mock'
TARGET_DFE = 'dfe'
TARGET_SIM = 'sim'

BENCHMARK_NONE = 'none'
BENCHMARK_BEST = 'best'
BENCHMARK_ALL_TO_ALL = 'all'

REP_CSV = 'csv'
REP_HTML  = 'html'

DIR_PATH_RESULTS = 'results'
DIR_PATH_LOG = 'logs'
DIR_PATH_RUNS = 'runs'

DSE_LOG_FILE = 'dse_run.log'

pd.options.display.float_format = '{:.2f}'.format

def preProcessBenchmark(benchDirPath):
  entries = []
  for f in os.listdir(benchDirPath):
    info = io.mminfo(os.path.join(benchDirPath, f))
    if info[0] == info[1]:
      info = list(info[1:])
    info.append(info[1] / info[0])
    info.insert(0, f.replace(r'.mtx', ''))
    info[1] = int(info[1])
    info[2] = int(info[2])
    entries.append(info)
  return sorted(entries, key=lambda x : x[-1], reverse=True)

def print_from_iterator(lines_iterator, logfile=None):
  output = ''
  if logfile:
    with open(logfile, 'w') as log:
      for line in lines_iterator:
        log.write(line)
        log.flush()
        output += line
  else:
    for line in lines_iterator:
      print line
      output += line
  return output

def execute(command, logfile=None, silent=True):
  if not silent:
    print 'Executing ', ' '.join(command)
  popen = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  output = None
  output_err = None
  if logfile or not silent:
    output = print_from_iterator(iter(popen.stdout.readline, b"") , logfile)
    err_log = logfile + '.err' if logfile else None
    output_err = print_from_iterator(iter(popen.stderr.readline,  b""), err_log)

  popen.wait()
  if popen.returncode != 0:
    if logfile:
      print 'log: ', logfile
      print 'errlog:  ', logfile + '.err'
    print colored("Error! '{0}' return code {1}".format(
      ' '.join(command), str(popen.returncode)) ,
      'red')

  return output, output_err

def runDse(benchFile, paramsFile, target, skipExecution=False):
  dseFile = "dse_out.json"
  if not skipExecution:
    execute(['../../build/main', benchFile, paramsFile], DSE_LOG_FILE)
  else:
    print '  --> Skip DSE run, load results from', dseFile
  params = []
  prjs = []
  architectures = []
  with open(dseFile) as f:
    data = json.load(f)
    for arch in data['best_architectures']:
      ps = arch['architecture_params']
      est_impl_ps = arch['estimated_impl_params']
      matrix = arch['matrices'][0]
      params.append(ps)
      # XXX Should check for identical architectures before assigning new ID
      prj_id = len(prjs)
      architectures.append(
          [ os.path.basename(matrix).replace('.mtx', ''),
            prj_id,
            int(ps['cache_size']), int(ps['input_width']),
            int(ps['num_pipes']), int(ps['max_rows']),
            # The model uses BRAM36, the McTools use BRAM18
            int(est_impl_ps['BRAMs']) * 2,
            int(est_impl_ps['LUTs']),
            int(est_impl_ps['FFs']),
            int(est_impl_ps['DSPs']),
            float(est_impl_ps['memory_bandwidth']),
            float(arch['estimated_gflops']), ])
      prjs.append(maxbuild.PrjConfig(ps, target, PRJ, prj_id, '../src/spmv/build/'))
  return prjs, architectures



def buildClient(target):
  print ' >> Building Client ----'
  execute(['make', '-C', '../../build/', 'test_spmv_' + target])


def runClient(benchmark, target, prj=None):
  print '     ---- Benchmarking Client ----'
  for p in benchmark:
    cmd = []
    if target == TARGET_DFE:
      cmd = ['bash', 'spark_dfe_run.sh', p]
    elif target == TARGET_SIM:
      cmd = ['bash', 'simrunner', '../../build/test_spmv_sim', p]
    elif target == TARGET_DFE_MOCK:
      cmd = ['bash', 'mockrunner', '../../build/test_spmv_dfe_mock', p]
    outF = 'runs/run_' + target + '_'
    if prj:
      cmd.append(str(prj.prj_id))
      outF += prj.buildName()
    else:
      outF += 'benchmark_best'
    outF += '_' + os.path.basename(p)
    print '      -->', p, 'outFile =', outF
    try:
      out = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      print '       ',e
      out = e.output

    mode = 'w'
    if prj:
      if os.path.exists(outF):
        os.remove(outF)
      mode = 'a'
    with open(outF, mode) as f:
      for line in out:
        f.write(line)


class Spark:

  def __init__(self, target, prjs, cppCompiler='g++'):
    self.target = target
    self.prjs =prjs
    self.cppCompiler = cppCompiler

  def runLibraryBuild(self, prjs, libName):
    print ' >> Building Library'

    interfaceFile = 'GeneratedImplementations.cpp'
    deviceO = 'SmpvDeviceInterface.o'
    maxfileO = 'maxfile.o'

    prj_includes = []
    obj_files = []
    if self.target != TARGET_DFE_MOCK:
      for p in prjs:
        objFile = p.name + '.o'
        execute(
            ['sliccompile', p.maxFileLocation(), objFile],
            logfile=p.logFile())
        prj_includes.append('-I' + p.resultsDir())
        obj_files.append(objFile)

    cmd =[
        self.cppCompiler,
      '-c',
      '-Wall',
      '-std=c++11',
      '-fPIC',
      '-I../runtime',
      ]

    # TODO move these checks in an earlier phase
    mcdir = os.getenv('MAXCOMPILERDIR')
    maxosdir = os.getenv('MAXELEROSDIR')
    if mcdir and maxosdir and self.target != TARGET_DFE_MOCK:
        cmd.extend([
            '-I' + mcdir + '/include',
            '-I' + mcdir + '/include/slic',
            '-I' + maxosdir + '/include'])

    cmd.extend(prj_includes)
    cmd.extend([
      interfaceFile,
      '-o',
      deviceO
      ])
    out = subprocess.check_output(cmd)

    cmd =[
        self.cppCompiler,
        '-fPIC',
        '--std=c++11',
        '-shared',
        '-Wl,-soname,{0}.0'.format(libName),
        '-o',
        libName]
    cmd.extend(obj_files + [deviceO])
    if mcdir and maxosdir and self.target != TARGET_DFE_MOCK:
      cmd.extend([
        '-L' + os.path.join(mcdir, 'lib'),
        '-L' + os.path.join(maxosdir, 'lib'),
        '-lmaxeleros',
        '-lslic',])

    cmd.extend(['-lm', '-lpthread'])
    execute(cmd)

    # copy the generated library
    libDir = '../lib-generated'
    if os.path.exists(libDir):
      shutil.rmtree(libDir)
    os.makedirs(libDir)
    shutil.copy(libName, libDir + '/{}.0'.format(libName))
    shutil.move(libName, libDir)

  def generateImplementationHeader(self, prjs):
    with open('GeneratedImplementations.cpp', 'w') as f:
      # Include maxfile headers
      if self.target != TARGET_DFE_MOCK:
        for p in prjs:
          f.write('#include <{0}.h>\n'.format(p.name))

      # Defines struct formats
      f.write('#include "{0}"\n'.format('GeneratedImplSupport.hpp'))

      f.write('using namespace cask::runtime;\n')

      f.write("""
          cask::runtime::SpmvImplementationLoader::SpmvImplementationLoader() : ImplementationLoader() {
          """)

      for i in range(len(prjs)):
        p = prjs[i]
        f.write('this->impls.push_back(')
        if self.target == TARGET_DFE_MOCK:
          f.write(
              'new GeneratedSpmvImplementationMock({0}, {1}, {2}, {3}, false));'.format(
                p.getParam('max_rows'),
                p.getParam('num_pipes'),
                p.getParam('cache_size'),
                p.getParam('input_width')))
        else:
          f.write(
              'new GeneratedSpmvImplementation({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}));'.format(
                p.prj_id,
                p.name,
                p.name + '_dramWrite',
                p.name + '_dramRead',
                p.getParam('max_rows'),
                p.getParam('num_pipes'),
                p.getParam('cache_size'),
                p.getParam('input_width'),
                p.name + '_dramReductionEnabled'))

      f.write('\n}')

  def runBuilds(self):

    print ' >> Building Hardware Implementations'
    if self.target != TARGET_DFE_MOCK:
      b = maxbuild.MaxBuildRunner()
      b.runBuilds(self.prjs)

    # library generation is sequential
    self.generateImplementationHeader(self.prjs)
    self.runLibraryBuild(self.prjs, 'libSpmv_' + self.target + '.so')
    buildClient(self.target)

  def runBenchmark(self, benchmark, benchmark_mode):
    if benchmark_mode == BENCHMARK_NONE:
      return

    if benchmark_mode == BENCHMARK_ALL_TO_ALL:
      for p in self.prjs:
        runClient(benchmark, self.target, p)
    else:
      runClient(benchmark, self.target)

def logTexTable(entries, fpath):
  rows = []

  float_prec = '.3f'

  # find maximum length
  length = 0
  for e in itertools.chain.from_iterable(entries):
    l = len(str(e))
    if type(e) is float:
      l = len(('{0:' + float_prec + '}').format(e))
    length = max(length, l)

  fmt = '{0:' + str(length) + '}'
  float_fmt = '{0:' + str(length) + float_prec + '}'

  for entry in entries:
    row = fmt.format(entry[0])
    for field in entry[1:]:
      f = fmt
      if type(field) is float:
        f = float_fmt
      row += ' &' + f.format(field)
    rows.append(row)

  table = '\\begin{{tabular}}{{{0}}} \n{1}\n\end{{tabular}}'.format(
      'l' * len(entries[0]),
      ' \\\\\n'.join(rows) + r' \\' )

  with open(fpath, 'w') as f:
    f.write(table)


def logDseResults(benchmark_df, arch_df):
  pd.set_option('display.max_columns', 500)
  pd.set_option('display.width', 1000)
  df = pd.merge(benchmark_df, arch_df, left_on='Matrix', right_on='Matrix')
  write_result('dse_matrix_arch.tex', df.to_latex())
  write_result('dse_matrix_arch.html', df.to_html())
  return df


def postProcessResults(prjs, benchmark, benchmark_df, arch_df, arch_build_df, dirpath):
  print colored('Post-processing results', 'red')
  # need to reconstruct a (matrix, architecture) relation from run files;
  # this relation also stores execution results (e.g.  bwidth, gflops)
  df = pd.DataFrame([], columns=['Id', 'Matrix', 'GFLOPs'])
  for p in os.listdir(dirpath):
    with open(os.path.join(dirpath, p)) as f:
      matrix = None
      archId = None
      gflops = None
      for l in f:
        m = re.match(r'Config ArchitectureId (\d*).*', l)
        if m:
          matrix = int(m.group(1))
        m = re.match(r'Param MatrixPath ([\w/-]*)', l)
        if m:
          archId = os.path.basename(m.group(1))
        m = re.match(r'Result Simple Gflops \(actual\)=(.*),', l)
        if m:
          gflops = float(m.group(1))
        if gflops and matrix and archId is not None:
          new_df = pd.DataFrame([[matrix, archId, gflops]], columns=['Id', 'Matrix', 'GFLOPs'])
          df = df.append(new_df, ignore_index=True)
          break

  # build a table compare est and measured results
  df1 = pd.merge(benchmark_df, df, left_on='Matrix', right_on='Matrix')
  df2 = pd.merge(df1, arch_df, left_on='Id', right_on='Id')
  df2 = pd.merge(df2, arch_build_df, left_on='Id', right_on='Id')

  # keep only some interesting columns and reorderd them
  df2 = df2[['Matrix_x', 'Order', 'Nonzeros', 'Nnz/row', 'Cx', 'k', 'Np', 'Cb', 'Logic %', 'DSP %', 'BRAM %', 'BWidth', 'GFLOPs_x', 'GFLOPs_y']]
  write_result('matrix_arch_before_after.tex', df2.to_latex(index=False))
  print arch_build_df
  print df2



def check_make_dir(dirname):
  if not os.path.exists(dirname):
    os.makedirs(dirname)

def make_clean_dir(dirname):
  if os.path.exists(dirname):
    shutil.rmtree(dirname)
  os.makedirs(dirname)

def write_result(fname, data):
  with open(os.path.join(DIR_PATH_RESULTS, fname), 'w') as f:
      f.write(data)


def build_html():
    matrices = []
    check_make_dir('matrices_html')

    for root, dirs, files in os.walk('matrices'):
      h = HTML()
      matrix = os.path.basename(root)
      if not dirs:
        print root, dirs, files
        h.p('Matrix: ' + matrix)
        sparsity_plot = None
        for f in files:
          if not f.endswith('.png'):
            with open(os.path.join(root, f)) as fin:
              h.p(fin.read(), style='white-space: pre-wrap;')
          else:
            p = h.p()
            p.img(src=matrix + '.png')
            sparsity_plot = os.path.join(root, f)

        path = 'matrices_html/' + matrix + '.html'
        with open(path, 'w') as fout:
          matrices.append(matrix + '.html')
          fout.write(str(h))
          shutil.copyfile(sparsity_plot, 'matrices_html/' + matrix + '.png')

    with open('matrices_html/index.html', 'w') as fout:
      h = HTML()
      h.p('matrices: ')
      l = h.ol

      for m in matrices:
        l.li.a(m, href=m)

      fout.write(str(h))


def main():

  parser = argparse.ArgumentParser(description='Run Spark DSE flow')
  parser.add_argument('-d', '--dse', action='store_true', default=False)
  parser.add_argument('-ds', '--dse-skip', action='store_true', default=False)
  parser.add_argument('-t', '--target', choices=[TARGET_DFE, TARGET_SIM, TARGET_DFE_MOCK], required=True)
  parser.add_argument('-p', '--param-file', required=True)
  parser.add_argument('-b', '--benchmark-dir', required=True)
  parser.add_argument('-st', '--build_start', type=int, default=None)
  parser.add_argument('-en', '--build_end', type=int, default=None)
  parser.add_argument('-bmst', '--benchmark_start', type=int, default=None)
  parser.add_argument('-bmen', '--benchmark_end', type=int, default=None)
  parser.add_argument('-cpp', '--cpp_compiler', default='g++')
  parser.add_argument('-bm', '--benchmarking-mode',
      choices=[BENCHMARK_BEST, BENCHMARK_ALL_TO_ALL, BENCHMARK_NONE],
      default=BENCHMARK_NONE)
  parser.add_argument('-rb', '--run-builds', default=False, action='store_true')
  parser.add_argument('-rep', '--reporting',
          choices=[REP_CSV, REP_HTML],
          default=REP_CSV)
  args = parser.parse_args()

  buildName = PRJ + '_' + args.target
  prjs = []

  ## Prepare some directories
  check_make_dir('results')
  check_make_dir('logs')
  if args.benchmarking_mode != BENCHMARK_NONE:
    make_clean_dir('runs')

  ## Run DSE pass
  prjs = []

  benchmark_df = pd.DataFrame(
          preProcessBenchmark(args.benchmark_dir),
          columns = ['Matrix', 'Order', 'Nonzeros', 'Format', 'Type', 'Pattern', 'Nnz/row'])

  if args.dse:
    print colored('Running DSE flow', 'red')
    # the DSE tool produces a JSON file with architectures to be built
    prjs, log_archs = runDse(args.benchmark_dir, args.param_file, args.target, args.dse_skip)
  else:
    # load default parameters values from param_file
    with open(args.param_file) as f:
      data = json.load(f)
      ps = {}
      for k, v in data['dse_params'].iteritems():
        ps[k] = str(v['default'])
    params = [maxbuild.PrjConfig(ps, args.target, PRJ, prj_id, '../src/spmv/build/')]

  arch_df = pd.DataFrame(log_archs,
          columns = ['Matrix', 'Id', 'Cx', 'k', 'Np', 'Cb', 'BRAMs', 'LUTs', 'FFs', 'DSPs', 'BWidth', 'GFLOPs'])
  merged_df = logDseResults(benchmark_df, arch_df)
  print merged_df

  p = os.path.abspath(args.benchmark_dir)
  benchmark = [ join(p, f) for f in listdir(p) if isfile(join(p,f)) ]
  if args.benchmark_start != None and args.benchmark_end != None:
    benchmark = benchmark[args.benchmark_start:args.benchmark_end]

  ps = prjs
  if args.build_start != None and args.build_end != None:
    ps = prjs[args.build_start:args.build_end]

  spark = Spark(args.target, ps, args.cpp_compiler)

  if args.run_builds:
    print colored('Running builds', 'red')
    spark.runBuilds()

  if args.target == TARGET_DFE:
    prj_info = []
    header = ['Id', 'Logic', 'Logic %', 'DSP', 'DSP %', 'BRAM', 'BRAM %']
    for p in ps:
      resUsage = p.getBuildResourceUsage()
      logic = resUsage['Logic utilization']
      dsps = resUsage['DSP blocks']
      brams = resUsage['Block memory (BRAM18)']
      prj_info.append([
        p.prj_id,
        logic[0], logic[0] / float(logic[1]) * 100,
        dsps[0], dsps[0] / float(dsps[1]) * 100,
        brams[0], brams[0] / float(brams[1]) * 100
        ])
    arch_build_df = pd.DataFrame(prj_info, columns = header)

  if args.benchmarking_mode != BENCHMARK_NONE:
    print colored('Running benchmark', 'red')
    spark.runBenchmark(benchmark, args.benchmarking_mode)

  # Post-process results
  if args.target == TARGET_DFE:
    postProcessResults(ps, benchmark,
        benchmark_df, arch_df, arch_build_df,
        DIR_PATH_RUNS)

  # Reporting
  if args.reporting == REP_HTML:
    print colored('Generating HTML reports', 'red')
    for p in benchmark:
      out, out_err = execute(['python', 'sparsegrind.py',
              '-f', 'mm', '-a', 'summary', p], silent=False)
      outputDir = os.path.join('matrices', os.path.basename(p).replace('.mtx', ''))
      summaryFile = os.path.join(outputDir, 'summary.csv')
      check_make_dir(outputDir)
      with open(summaryFile, 'w') as f:
        f.write(out)
      execute(['python', 'sparsegrind.py',
              '-f', 'mm', '-a', 'plot', p], silent=False)
      shutil.copy('sparsity.png', outputDir)

    build_html()

    # TODO also need to add hardware / simulation results to report
    # matrix_sim_run=${matrix_dir}/sim_run.csv
    # cd scripts && bash simrunner ../build/test_spmv_sim ../${f} >> ../${matrix_sim_run} && cd ..

    bs = BeautifulSoup(merged_df.to_html(), 'html.parser')
    for row in bs.findAll('tr'):
        cols = row.findAll('td')
        if cols:
            matrixName = cols[0].string
            new_tag = bs.new_tag('a', href='matrices/' + matrixName + '.html')
            new_tag.string = matrixName
            cols[0].string = ''
            cols[0].append(new_tag)
    with open('matrices_html/matrix_index.html', 'w') as f:
        f.write(str(bs))


if __name__ == '__main__':
  main()
