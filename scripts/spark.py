"""Implements the main DSE loop in spark."""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import multiprocessing

from os import listdir
from os.path import isfile, join
from multiprocessing import Pool

from subprocess import call

class PrjConfig:
  def __init__(self, p, t, n):
    self.params = p
    self.target = t
    self.name = n
    self.buildRoot='../src/spmv/build/'

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

  def buildTarget(self):
    if self.target == 'sim':
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
    return self.target == 'sim'


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


def runLibraryBuild(prj):
  print '     ---- Building Library ----'

  interfaceFile = '../src/spmv/src/SpmvDeviceInterface.cpp'
  deviceO = 'SmpvDeviceInterface.o'
  maxfileO = 'maxfile.o'

  out = subprocess.check_output([
    'sliccompile',
    prj.maxFileLocation(),
    maxfileO])

  mcdir = os.getenv('MAXCOMPILERDIR')
  maxosdir = os.getenv('MAXELEROSDIR')
  cmd =[
	  'g++',
    '-c',
    '-Wall',
    '-std=c++11',
    '-I../include',
    '-I' + prj.resultsDir(),
    '-I' + mcdir + '/include',
    '-I' + mcdir + '/include/slic',
    '-I' + maxosdir + '/include',
    interfaceFile,
    '-o',
    deviceO
    ]
  out = subprocess.check_output(cmd)

  cmd =[
      'g++',
      '-fPIC',
      '--std=c++11',
      '-shared',
      '-Wl,-soname,{0}.0'.format(prj.libName()),
      '-o',
      prj.libName(),
      maxfileO,
      deviceO,
      '-L' + os.path.join(mcdir, 'lib'),
      '-L' + os.path.join(maxosdir, 'lib'),
      '-lmaxeleros',
      '-lslic',
      '-lm',
      '-lpthread']
  out = subprocess.check_output(cmd)
  print out

  # copy the generated library
  libDir = '../lib-generated'
  if os.path.exists(libDir):
    shutil.rmtree(libDir)
  os.makedirs(libDir)
  shutil.copy(prj.libName(), libDir + '/{}.0'.format(prj.libName()))
  shutil.copy(prj.libName(), libDir + '/libSpmv_sim.so')
  shutil.move(prj.libName(), libDir)

  # do a bit of cleanup
  if os.path.exists(maxfileO):
    os.remove(maxfileO)
  if os.path.exists(deviceO):
    os.remove(deviceO)


def buildClient(prj):
  print '     ---- Building Client ----'
  out = subprocess.check_output(['make',
    '-C',
    '../build/',
    'test_spmv_' + prj.target])


def runClient(prj, benchmark):
  print '     ---- Benchmarking Client ----'
  for p in benchmark:
    outF = 'run_' + prj.buildName() + '_' + os.path.basename(p)
    print '      -->', p, 'outFile =', outF
    cmd = ['bash', 'spark_dfe_run.sh', p]
    if prj.sim():
      cmd = ['bash',
          'simrunner',
          '../build/test_spmv_sim',
          p]
    try:
      out = subprocess.check_output(cmd)
    except subprocess.CalledProcessError as e:
      print '       ',e
      out = e.output
    with open(outF, 'w') as f:
      for line in out:
        f.write(line)


def runBuilds(prjs, benchmark):
  # TODO run MC builds in parallel
  pool = Pool(4)
  pool.map(runMaxCompilerBuild, prjs)
  pool.close()
  pool.join()

  # Client builds and benchmarking is sequential
  for p in prjs:
    runLibraryBuild(p)
    buildClient(p)
    runClient(p, benchmark)


def main():

  parser = argparse.ArgumentParser(description='Run Spark DSE flow')
  parser.add_argument('-d', '--dse', action='store_true', default=False)
  parser.add_argument('-t', '--target', choices=['dfe', 'sim'], required=True)
  parser.add_argument('-p', '--param-file', required=True)
  parser.add_argument('-b', '--benchmark-dir', required=True)
  parser.add_argument('-m', '--max-builds', type=int)
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

  prjs = [PrjConfig(p, args.target, PRJ) for p  in params]

  print 'Running builds'
  p = os.path.abspath(args.benchmark_dir)
  benchmark = [ join(p, f) for f in listdir(p) if isfile(join(p,f)) ]


  ps = prjs[:args.max_builds] if args.max_builds else prjs

  runBuilds(ps, benchmark)


if __name__ == '__main__':
  main()
