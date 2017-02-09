"""This module contains useful functions and classes for creating and managing MaxCompiler builds"""
import os
import multiprocessing
import subprocess
from multiprocessing import Pool
from termcolor import colored
from os.path import isfile, join

TARGET_DFE_MOCK = 'dfe_mock'
TARGET_DFE = 'dfe'
TARGET_SIM = 'sim'
DIR_PATH_LOG = 'logs'

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
      ' '.join(command), str(popen.returncode)) , 'red')

  return output, output_err


def runMaxCompilerBuild(prj):
  print prj, prj.buildTarget(), prj.buildName(), # prj.name
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
