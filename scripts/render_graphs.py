import re
import os
import pprint
import numpy as np
import matplotlib.pyplot as plt
import collections
from matplotlib.backends.backend_pdf import PdfPages

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

def sort_by_avg_of_value(elems):
  sorted_x = sorted(
          elems.items(),
          key = lambda x : np.mean(x[1]))
  return collections.OrderedDict(sorted_x)

def extract_from_group(field, group):
  new_group = collections.defaultdict(list)
  for key, value in group.iteritems():
    for e in value:
      new_group[key].append(getattr(e, field))
  return new_group

def split_dict_by_size(elems, size):
  dicts = []
  pos = 0
  while pos < len(elems):
    dicts.append(dict(elems.items()[pos:pos+size]))
    pos += size
  return dicts

def bar_plot(pdf, group, group_names):
  fig, ax = plt.subplots()

  bar_groups = len(group)
  num_groups = len(group.itervalues().next())

  ind = np.arange(num_groups)
  colors = ['r', 'y', 'g', 'b', 'm', 'c']

  width = 0.15
  k = 0
  names = []
  for key, value in group.iteritems():
    print  np.mean(value)
    c = colors[k % len(colors)]
    ax.bar(ind + k * width, value, width,
            linewidth=0,
            alpha=0.6, color=c)
    names.append(key)
    k += 1

  plt.xticks(ind + width * bar_groups / 2, list(group_names))
  locs, labels = plt.xticks()
  plt.setp(labels, rotation=90, fontsize=5)

  plt.legend(names, loc='upper center', fontsize=8)
  # plt.tight_layout()

  pdf.savefig()
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
  gflops_per_matr_per_project = sort_by_avg_of_value(extract_from_group('gflops_est', group_by_project))

  # Render graphs
  pp = pprint.PrettyPrinter(indent=2)
  pp.pprint(matrices)
  pp.pprint(dict(group_by_project))
  pp.pprint(dict(gflops_per_matr_per_project))

  pdf = PdfPages('gflops_per_matrix.pdf')
  i = 0
  for d in split_dict_by_size(gflops_per_matr_per_project, 5):
    bar_plot(pdf, d, matrices)
    i += 1
  pdf.close()


if __name__ == '__main__':
  main()
