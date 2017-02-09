import re
import os
import pprint
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import collections
from matplotlib.backends.backend_pdf import PdfPages

import seaborn as sns

from sparsegrind import main

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


def main():
  runResults = []
  # Traverse files, extract matrix, architecture and params
  for f in [f for f in os.listdir('.') if os.path.isfile(f)]:
      if f.startswith('run_Spmv'):
        runResults.append(RunResult(f))

  df = pd.DataFrame([[r.prj, r.matrix, r.gflops_est] for r in runResults])
  grouped = df.groupby(0)
  groups = []
  names = []
  for name, group in grouped:
    group.set_index(1, inplace=True)
    # group.sort_index(inplace=True)
    groups.append(group[2])
    names.append(name)

  new_df = pd.concat(groups, axis=1)
  new_df.columns = names

  sns.set_style("white")
  sns.set_palette(sns.color_palette("cubehelix", 13))
  bar = new_df.plot(kind='bar')
  sns.despine()
  fig = bar.get_figure()
  fig.set_size_inches(15, 15)
  fig.tight_layout()
  fig.savefig('est_gflops.pdf')
  # plt.show()


if __name__ == '__main__':
  main()
