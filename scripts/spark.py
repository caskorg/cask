"""Implements the main DSE loop in spark."""
import json
import argparse
import subprocess
import os

from subprocess import call

class PrjConfig:
  def __init__(self, p, t, n):
    self.params = p
    self.target = t
    self.name = n

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


def runBuild(prj):
  buildName = prj.buildName()
  buildRoot='../src/spmv/build/'
  buildDir = os.path.join(buildRoot, buildName)
  maxFileLocation = os.path.join(buildDir, 'results', prj.maxfileName())
  maxFileTarget = os.path.join(buildName, 'results', prj.maxfileName())

  buildParams = "target={0} buildName={1} maxFileName={2}".format(
      prj.buildTarget(), buildName, prj.name)
  maxBuildParams = prj.maxBuildParams()

  print '  Running build'
  print '    buildName        = ', buildName
  print '    buildDir         = ', buildDir
  print '    maxFileLocation  = ', maxFileLocation
  print '    maxFileTarget    = ', maxFileTarget
  print '    buildParams      = ', buildParams
  print '    maxBuildParams   = ', maxBuildParams

  cmd = [
      'make',
      'MAX_BUILDPARAMS="' + maxBuildParams + buildParams,
      "-C",
      buildRoot,
      maxFileTarget]
  print cmd
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  while p.poll() is None:
      l = p.stdout.readline()
      print l,
  print p.stdout.read()


def runBuilds(prjs):
  for p in  prjs:
    runBuild(p)


def main():

  parser = argparse.ArgumentParser(description='Run Spark DSE flow')
  parser.add_argument('-d', '--dse', action='store_true', default=False)
  parser.add_argument('-t', '--target', choices=['dfe', 'sim'], required=True)
  parser.add_argument('-p', '--param-file', required=True)
  parser.add_argument('-b', '--benchmark-dir', required=True)
  args = parser.parse_args()

  PRJ = 'Spmv'
  buildName = PRJ + '_' + args.target
  prjs = []

  ## Run DSE pass
  if args.dse:
    print 'Running DSE flow'
    # the DSE tool produces a JSON file with architectures to be built
    params = runDse(args.benchmark_dir, args.param_file)
    # builds = [buildName + b for b in builds]
    prjs = [PrjConfig(p, args.target, PRJ) for p  in params]
  else:
    # TODO some default build
    params = []

  print 'Running builds'
  print prjs

  runBuilds(prjs)


if __name__ == '__main__':
  main()
