import re
import os
import pprint
import numpy as np
import matplotlib.pyplot as plt
import collections

class Architecture:

  def __init__(self, params):
    self.params = {}

class RunResult:

  def __init__(self, outputFile):
    self.prj = None
    self.matrix = None
    self.gflops_est = 0
    self.outputFile = outputFile
    self.loadData(outputFile)

  def loadData(self, outputFile):
    m = re.search(r'run_(\w*maxrows\d*)_([\w-]*).mtx', outputFile)
    if m:
      self.prj = m.group(1)
      self.matrix = m.group(2)

    with open(outputFile, 'r') as f:
      for l in f:
        m = re.search(r'Result Simple Gflops \(est\)=(\d*(.\d*)?)', l)
        if m:
          self.gflops_est = float(m.group(1))

  def __str__(self):
    return 'gflops_est = {0}, matrix = {1}, prj = {2}'.format(
        self.gflops_est, self.matrix, self.prj)

  def __repr__(self):
    return self.__str__()

def all_unique(field, elems):
  return sorted(list(set([getattr(e, field) for e in elems])))

def group_by_field(field, elems):
  grp = collections.defaultdict(list)
  for e in elems:
    grp[getattr(e, field)].append(e)
  for key, value in grp.iteritems():
      value.sort(key=lambda x: x.matrix)
  return grp

def extract_from_group(field, group):
  new_group = collections.defaultdict(list)
  for key, value in group.iteritems():
    for e in value:
      new_group[key].append(getattr(e, field))
  return new_group


def bar_plot(group, file_name, group_names):
  fig, ax = plt.subplots()

  bar_groups = len(group)
  num_groups = len(group.itervalues().next())

  ind = np.arange(num_groups)
  colors = ['r', 'y', 'g', 'b', 'm', 'c']

  width = 0.1
  k = 0
  names = []
  for key, value in group.iteritems():
    print value
    ax.bar(ind + k * width, value, width,
            alpha=0.4, color=colors[k % len(colors)])
    names.append(key)
    k += 1

  plt.xticks(ind + width * bar_groups / 2, list(group_names))
  locs, labels = plt.xticks()
  plt.setp(labels, rotation=90, fontsize=5)

  plt.legend(names, loc='upper center', fontsize=8)
  # plt.tight_layout()

  fig.savefig(file_name)
  plt.close(fig)


def main():
  runResults = []
  # Traverse files, extract matrix, architecture and params
  for f in [f for f in os.listdir('.') if os.path.isfile(f)]:
      if f.startswith('run_Spmv'):
        runResults.append(RunResult(f))

  # group by matrix
  matrices = all_unique('matrix', runResults)
  projects = all_unique('prj', runResults)

  group_by_project = group_by_field('prj', runResults)
  gflops_per_matr_per_project = extract_from_group('gflops_est', group_by_project)

  # Render graphs
  pp = pprint.PrettyPrinter(indent=2)
  # pp.pprint(runResults)
  pp.pprint(matrices)
  pp.pprint(dict(group_by_project))
  pp.pprint(dict(gflops_per_matr_per_project))
  # by_matrix = [ r.prj

  # for each group of 4 do a plot
  bar_plot(gflops_per_matr_per_project, 'gflops_per_matrix.pdf', matrices)


if __name__ == '__main__':
  main()
