"""Implements the main DSE loop in spark."""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import multiprocessing
import pprint

from os import listdir
from os.path import isfile, join
from multiprocessing import Pool

from subprocess import call


TARGET_DFE_MOCK = 'dfe_mock'
TARGET_DFE = 'dfe'
TARGET_SIM = 'sim'


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


class RunResult:

  def __init__(self, prj, matrix, outputFile):
    self.prj = prj
    self.matrix = matrix
    self.loadData(outputFile)

  def loadData(self, outputFile):
    with open(outputFile, 'r') as f:
      for l in f:
        m = re.search(r'Result Simple Gflops \(est\)=(\d*(.\d*)?)', l)
        if m:
          self.gflops_est = float(m.group(1))

  def __str__(self):
    return 'gflops_est = {0}, matrix = {1}, prj = {2}'.format(
        self.gflops_est, self.matrix, self.prj.buildName())

  def __repr__(self):
    return self.__str__()

def runDse(benchFile, paramsFile):
  dseLog = subprocess.check_output(
      ["../build/main", benchFile, paramsFile])
  dseFile = "dse_out.json"
  params = []
  with open(dseFile) as f:
    data = json.load(f)
    for arch in data['best_architectures']:
      params.append(arch['architecture_params'])
  return params


def runMaxCompilerBuild(prj):
  buildParams = "target={0} buildName={1} maxFileName={2} ".format(
      prj.buildTarget(), prj.buildName(), prj.name)

  print '  Running build'

  cmd = [
      'make',
      'MAX_BUILDPARAMS="' + prj.maxBuildParams() + buildParams + '"',
      "-C",
      prj.buildRoot,
      prj.maxFileTarget()]

  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  print '     ---- Building MaxFile ----'
  while p.poll() is None:
      l = p.stdout.readline()
      sys.stdout.write('      ')
      sys.stdout.write(l)
  for l in p.stdout.read().split('\n'):
    if l:
      print '     ', l.rstrip()
  print ''

  forceTimingScore(prj.maxFileLocation())


def forceTimingScore(maxfile):
  # Might want to find a way to do this in python...
  oldts=subprocess.check_output(['grep', 'TIMING_SCORE', maxfile])
  cmd= [
    'sed', '-i', '-e',
    r's/PARAM(TIMING_SCORE,.*)/PARAM(TIMING_SCORE, 0)/',
    maxfile]
  call(cmd)
  newts = subprocess.check_output(['grep', 'TIMING_SCORE', maxfile])
  print '      Changing timing score: {0} --> {1}'.format(oldts.strip(), newts.strip())



def buildClient(target):
  print '     ---- Building Client ----'
  out = subprocess.check_output(['make',
    '-C',
    '../build/',
    'test_spmv_' + target])


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
    if prj:
      cmd.append(str(prj.prj_id))
      outF = 'run_' + prj.buildName()
    else:
      outF = 'run_benchmark_best'
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

    if prj:
      prj.runResults.append(RunResult(prj, p, outF))

class Spark:

  def __init__(self, target):
    self.target = target

  def runLibraryBuild(self, prjs, libName):
    print '     ---- Building Library ----'

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
    if mcdir and maxosdir:
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
    if mcdir and maxosdir:
      cmd.extend([
        '-L' + os.path.join(mcdir, 'lib'),
        '-L' + os.path.join(maxosdir, 'lib'),
        '-lmaxeleros',
        '-lslic',])

    cmd.extend(['-lm', '-lpthread'])
    out = subprocess.check_output(cmd)
    print out

    # # copy the generated library
    libDir = '../lib-generated'
    if os.path.exists(libDir):
      shutil.rmtree(libDir)
    os.makedirs(libDir)
    shutil.copy(libName, libDir + '/{}.0'.format(libName))
    shutil.move(libName, libDir)

    # # do a bit of cleanup
    # if os.path.exists(maxfileO):
      # os.remove(maxfileO)
    # if os.path.exists(deviceO):
      # os.remove(deviceO)


  def generateImplementationHeader(self, prjs):
    with open('GeneratedImplementations.cpp', 'w') as f:
      # Include maxfile headers
      if self.target != TARGET_DFE_MOCK:
        for p in prjs:
          f.write('#include <{}.h>\n'.format(p.name))

      # Defines struct formats
      f.write('#include <Spark/{}>\n'.format('GeneratedImplSupport.hpp'))

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

  def runBuilds(self, prjs, benchmark, benchmark_all_to_all, sim):

    if self.target != TARGET_DFE_MOCK:
      # run MC builds in parallel
      pool = Pool(4)
      pool.map(runMaxCompilerBuild, prjs)
      pool.close()
      pool.join()

    # library generation is sequential
    self.generateImplementationHeader(prjs)
    self.runLibraryBuild(prjs, 'libSpmv_' + self.target + '.so')

    buildClient(self.target)

    if benchmark_all_to_all:
      for p in prjs:
        runClient(benchmark, self.target, p)
    else:
      runClient(benchmark, self.target)

    pp = pprint.PrettyPrinter(indent=2)
    for p in prjs:
      pp.pprint(p.runResults)


def main():

  parser = argparse.ArgumentParser(description='Run Spark DSE flow')
  parser.add_argument('-d', '--dse', action='store_true', default=False)
  parser.add_argument('-t', '--target', choices=[TARGET_DFE, TARGET_SIM, TARGET_DFE_MOCK], required=True)
  parser.add_argument('-p', '--param-file', required=True)
  parser.add_argument('-b', '--benchmark-dir', required=True)
  parser.add_argument('-m', '--max-builds', type=int)
  parser.add_argument('-bm', '--benchmarking-mode', choices=['best-fit', 'all'], default='best-fit')
  args = parser.parse_args()

  PRJ = 'Spmv'
  buildName = PRJ + '_' + args.target
  prjs = []

  ## Run DSE pass
  params = []
  if args.dse:
    print 'Running DSE flow'
    # the DSE tool produces a JSON file with architectures to be built
    params = runDse(args.benchmark_dir, args.param_file)
    # builds = [buildName + b for b in builds]
  else:
    with open(args.param_file) as f:
      data = json.load(f)
      ps = {}
      for k, v in data['dse_params'].iteritems():
        ps[k] = str(v['default'])
    params = [ps]

  prjs = []
  for i in range(len(params)):
    prjs.append(PrjConfig(params[i], args.target, PRJ, i))

  print 'Running builds'
  p = os.path.abspath(args.benchmark_dir)
  benchmark = [ join(p, f) for f in listdir(p) if isfile(join(p,f)) ]


  ps = prjs[:args.max_builds] if args.max_builds else prjs

  spark = Spark(args.target)
  spark.runBuilds(
          ps,
          benchmark,
          args.benchmarking_mode == 'all',
          args.target == 'sim')


if __name__ == '__main__':
  main()
