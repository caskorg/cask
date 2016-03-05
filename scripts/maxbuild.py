"""This module contains useful functions and classes for creating and managing MaxCompiler builds"""
import os
import multiprocessing
from multiprocessing import Pool
from os.path import isfile, join

class PrjConfig:
  def __init__(self, p, t, n, prj_id, buildRoot):
    self.params = p
    self.target = t
    self.name = n + '_' + str(prj_id)
    self.buildRoot = buildRoot
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

  def logFile(self):
    return self.buildName() + '.log'

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


class MaxBuildRunner:
  def __init__(self):
    pass

  def runBuilds(self, prjs):
    # run MC builds in parallel
    pool = Pool(6)
    pool.map(runMaxCompilerBuild, prjs)
    pool.close()
    pool.join()


def runMaxCompilerBuild(prj):
  print 'Here'
  print prj, prj.buildTarget(), prj.buildName(), # prj.name
  print 'Here2'
  buildParams = "target={0} buildName={1} maxFileName={2} ".format(
      prj.buildTarget(), prj.buildName(), prj.name)
  print '  --> Building MaxFile ', prj.buildName(), '\n',
  print 'Here'

  prjLogFile = os.path.join(DIR_PATH_LOG, prj.buildName() + '.log')
  execute(
      ['make', 'MAX_BUILDPARAMS="' + prj.maxBuildParams() + buildParams + '"',
      "-C", prj.buildRoot, prj.maxFileTarget()],
      prjLogFile)

  execute(['sed', '-i', '-e', r's/PARAM(TIMING_SCORE,.*)/PARAM(TIMING_SCORE, 0)/',
    prj.maxFileLocation()
    ])
