"""Implements the main DSE loop in spark."""
import argparse
import itertools
import json
import multiprocessing
import os
import pprint
import re
import shutil
import subprocess
import sys
import pandas as pd
from tabulate import tabulate

from multiprocessing import Pool
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

DIR_PATH_RESULTS = 'results'
DIR_PATH_LOG = 'logs'

DSE_LOG_FILE = 'dse_run.log'

class PrjConfig:
  def __init__(self, p, t, n, prj_id):
    self.params = p
    self.target = t
    self.name = n + '_' + str(prj_id)
    self.buildRoot='../src/spmv/build/'
    self.prj_id = prj_id
    self.runResults = []

  def buildName(self):
    bn = self.name + '_' + self.target
    for k, v in self.params.iteritems():
      bn += '_' + k.replace('_', '') + v
    return bn

  def maxfileName(self):
    return self.name + '.max'

  def __str__(self):
    return 'params = {0}, target = {1}, name = {2}'.format(
        self.params, self.target, self.name)

  def __repr__(self):
    return self.__str__()

  def maxBuildParams(self):
    params = ''
    for k, v in self.params.iteritems():
      params += k + '=' + v + ' '
    return params

  def getParam(self, p):
    return self.params[p]

  def buildTarget(self):
    if self.target == TARGET_SIM:
      return 'DFE_SIM'
    return 'DFE'

  def buildDir(self):
    return os.path.join(self.buildRoot, self.buildName())

  def maxFileLocation(self):
    return os.path.join(self.resultsDir(), self.maxfileName())

  def maxFileTarget(self):
    return os.path.join(self.buildName(), 'results', self.maxfileName())

  def resultsDir(self):
    return os.path.join(self.buildDir(), 'results')

  def libName(self):
    return 'lib{0}.so'.format(self.buildName())

  def sim(self):
    return self.target == TARGET_SIM

  def resourceUsageReportDir(self):
    return os.path.join(self.buildDir(), 'src_annotated')

  def buildLog(self):
    return os.path.join(self.buildDir(), '_build.log')

  def getBuildResourceUsage(self):
    usage = {}
    with open(self.buildLog()) as f:
      while True:
        l = f.readline()
        if l.find('FINAL RESOURCE USAGE') != -1:
          for i in range(7):
            m = re.match(r'.*PROGRESS:(.*):\s*(\d*)\s/\s(\d*).*', f.readline())
            usage[m.group(1).strip()] = (int(m.group(2)), int(m.group(3)))
          break
    return usage


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
  if logfile:
    with open(logfile, 'w') as log:
      for line in lines_iterator:
        log.write(line)
        log.flush()
  else:
    for line in lines_iterator:
      print line

def execute(command, logfile=None, silent=True):
  if not silent:
    print 'Executing ', ' '.join(command)
  popen = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  if logfile:
    print_from_iterator(iter(popen.stdout.readline, b"") , logfile)
    print_from_iterator(iter(popen.stderr.readline,  b""), logfile + '.err')

  popen.wait()
  if popen.returncode != 0:
    if logfile:
      print 'log: ', logfile
      print 'errlog:  ', logfile + '.err'
    print colored("Error! '{0}' return code {1}".format(
      ' '.join(command), str(popen.returncode)) ,
      'red')

def runDse(benchFile, paramsFile, target, skipExecution=False):
  dseFile = "dse_out.json"
  if not skipExecution:
    execute(['../build/main', benchFile, paramsFile], DSE_LOG_FILE)
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
            int(est_impl_ps['BRAMs']),
            int(est_impl_ps['LUTs']),
            int(est_impl_ps['FFs']),
            int(est_impl_ps['DSPs']),
            float(est_impl_ps['memory_bandwidth']),
            float(arch['estimated_gflops']), ])
      prjs.append(PrjConfig(ps, target, PRJ, prj_id))
  return prjs, architectures


def runMaxCompilerBuild(prj):
  buildParams = "target={0} buildName={1} maxFileName={2} ".format(
      prj.buildTarget(), prj.buildName(), prj.name)
  print '  --> Building MaxFile ', prj.buildName(), '\n',

  prjLogFile = os.path.join(DIR_PATH_LOG, prj.buildName() + '.log')
  execute(
      ['make', 'MAX_BUILDPARAMS="' + prj.maxBuildParams() + buildParams + '"',
      "-C", prj.buildRoot, prj.maxFileTarget()],
      prjLogFile)

  execute(['sed', '-i', '-e', r's/PARAM(TIMING_SCORE,.*)/PARAM(TIMING_SCORE, 0)/',
    prj.maxFileLocation()
    ])


def buildClient(target):
  print ' >> Building Client ----'
  execute(['make', '-C', '../build/', 'test_spmv_' + target])


def runClient(benchmark, target, prj=None):
  print '     ---- Benchmarking Client ----'
  for p in benchmark:
    cmd = []
    if target == TARGET_DFE:
      cmd = ['bash', 'spark_dfe_run.sh', p]
    elif target == TARGET_SIM:
      cmd = ['bash', 'simrunner', '../build/test_spmv_sim', p]
    elif target == TARGET_DFE_MOCK:
      cmd = ['bash', 'mockrunner', '../build/test_spmv_dfe_mock', p]
    outF = 'run_' + target + '_'
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

  def __init__(self, target, prjs):
    self.target = target
    self.prjs =prjs

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
        out = subprocess.check_output([
          'sliccompile',
          p.maxFileLocation(),
          objFile])
        prj_includes.append('-I' + p.resultsDir())
        obj_files.append(objFile)

    cmd =[
      'g++',
      '-c',
      '-Wall',
      '-std=c++11',
      '-fPIC',
      '-I../include',
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
        'g++',
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
      f.write('#include <Spark/{0}>\n'.format('GeneratedImplSupport.hpp'))

      f.write('using namespace spark::runtime;\n')

      f.write("""
          spark::runtime::SpmvImplementationLoader::SpmvImplementationLoader() : ImplementationLoader() {
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
              'new GeneratedSpmvImplementation({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}));'.format(
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
      # run MC builds in parallel
      pool = Pool(6)
      pool.map(runMaxCompilerBuild, self.prjs)
      pool.close()
      pool.join()

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
  print df
  write_result('dse_matrix_arch.tex', df.to_latex())
  write_result('dse_matrix_arch.html', df.to_html())


def check_make_dir(dirname):
  if not os.path.exists(dirname):
    os.makedirs(dirname)


def write_result(fname, data):
  with open(os.path.join(DIR_PATH_RESULTS, fname), 'w') as f:
      f.write(data)


def main():

  parser = argparse.ArgumentParser(description='Run Spark DSE flow')
  parser.add_argument('-d', '--dse', action='store_true', default=False)
  parser.add_argument('-ds', '--dse-skip', action='store_true', default=False)
  parser.add_argument('-t', '--target', choices=[TARGET_DFE, TARGET_SIM, TARGET_DFE_MOCK], required=True)
  parser.add_argument('-p', '--param-file', required=True)
  parser.add_argument('-b', '--benchmark-dir', required=True)
  parser.add_argument('-st', '--build_start', type=int, default=None)
  parser.add_argument('-en', '--build_end', type=int, default=None)
  parser.add_argument('-bm', '--benchmarking-mode',
      choices=[BENCHMARK_BEST, BENCHMARK_ALL_TO_ALL, BENCHMARK_NONE],
      default=BENCHMARK_NONE)
  parser.add_argument('-rb', '--run-builds', default=False, action='store_true')
  args = parser.parse_args()

  buildName = PRJ + '_' + args.target
  prjs = []

  ## Prepare some directories
  check_make_dir('results')
  check_make_dir('logs')

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
    params = [PrjConfig(ps, args.target, PRJ, prj_id)]

  arch_df = pd.DataFrame(log_archs,
          columns = ['Matrix', 'Id', 'Cx', 'k', 'Np', 'Cb', 'BRAMs', 'LUTs', 'FFs', 'DSPs', 'BWidth', 'GFLOPs'])
  logDseResults(benchmark_df, arch_df)

  p = os.path.abspath(args.benchmark_dir)
  benchmark = [ join(p, f) for f in listdir(p) if isfile(join(p,f)) ]

  ps = prjs
  if args.build_start != None and args.build_end != None:
    ps = prjs[args.build_start:args.build_end]

  spark = Spark(args.target, ps)

  if args.run_builds:
    print colored('Running builds', 'red')
    spark.runBuilds()

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
  print tabulate(prj_info, headers=header, floatfmt=".1f")

  if args.benchmarking_mode != BENCHMARK_NONE:
    print colored('Running benchmark', 'red')
    spark.runBenchmark(benchmark, args.benchmarking_mode)

if __name__ == '__main__':
  main()
