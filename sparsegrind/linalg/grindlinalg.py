import time
import argparse
import scipy
from scipy import io, sparse
from scipy.sparse import linalg
import numpy as np
import os
import sys
from termcolor import colored
from tabulate import tabulate

import scipy.sparse.linalg as spla

def runSolver(solver, system, rhs, squeeze=True):
  sol = solver(system, rhs)
  got = rhs
  if squeeze:
    got = np.squeeze(rhs.toarray())
  check = system.dot(sol).transpose()
  print np.linalg.norm(np.subtract(got, check))

def l2norm(exp, got):
    return np.linalg.norm(np.subtract(got, exp))

def check(msg, exp, got):
  print msg, 'L2 norm:', l2norm(got, exp)

class SolverInfo(object):
  def __init__(self):
    self.iterations = 0

  def printer(self, x):
    # print x
    self.iterations += 1


def runCgSolvers(matrix_path, vec_path, use_precon=True, max_size=None):
  system = sparse.csr_matrix(io.mmread(matrix_path))
  rhs = io.mmread(vec_path)
  n = system.shape[0]
  if max_size and n > max_size:
    print colored('System too large ignoring')
    sys.exit(1)
  print colored('Testing system ' + matrix_path, 'red')
  print io.mminfo(matrix_path), io.mminfo(vec_path), float(system.nnz) / n
  # -- Direct solver
  t0 = time.time()
  dir_sol = scipy.sparse.linalg.spsolve(system, rhs)
  t1 = time.time()
  directSolveTime = t1 - t0
  if type(rhs) is not np.ndarray:
    rhs = rhs.toarray()
  check('Direct Solver', np.squeeze(rhs), system.dot(dir_sol))
  # -- CG Solver
  resultsList = []
  for solver in [spla.bicgstab]:  # , spla.cg, spla.gmres]: #, spla.bicg, spla.cgs, spla.minres, spla.qmr]:
    # We can try these out in higher ranges, but for now it seems that ideal values
    # are around 100 for the fill_factor and 1e-12 for the drop tollerance
    for fill_factor in [10, 100]:  # 10000, 1000, 100, 10]:
      for drop_tol in [1e-6, 1e-12]:  # , 1e-8, 1e-6]:
        t0 = time.time()
        approx_inv = scipy.sparse.linalg.spilu(
          sparse.csc_matrix(system),
          fill_factor=fill_factor,
          drop_tol=drop_tol)
        t1 = time.time()
        preconTime = t1 - t0
        # print approx_inv
        M = scipy.sparse.linalg.LinearOperator(
          (n, n), lambda x: approx_inv.solve(x))
        # M = scipy.sparse.linalg.inv(approx_inv)

        precon = use_precon and solver not in [spla.bicg, spla.minres, spla.qmr]
        solverInfo = SolverInfo()
        t0 = time.time()
        if precon:
          sol = solver(system, rhs, M=M, maxiter=2000, callback=solverInfo.printer)
        else:
          sol = solver(system, rhs, maxiter=n)
        t1 = time.time()
        iterativeSolverTime = t1 - t0
        result = sol[0]
        # print 'Direct solution', dir_sol
        # print 'Iterative', result
        resultsList.append(
          [
            fill_factor, drop_tol, solver, precon, solverInfo.iterations,
            directSolveTime, preconTime, iterativeSolverTime, preconTime + iterativeSolverTime,
            l2norm(np.squeeze(rhs), system.dot(result))
          ]
        )
  headers = ['fill_factor', 'drop_tol', 'solver', 'precon', 'iterations', 'direct(s)', 'precon(s)', 'it(s)', 'itTotal(s)', 'l2norm']
#  print tabulate(resultsList, headers)
  return resultsList, headers


def main():
  parser = argparse.ArgumentParser(
    description='Benchmark sparse linear solvers',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('-m', '--matrix-path',
                      help='Path to matrix to use (in Matrix Market Format')
  parser.add_argument('-b',
                      help='Path to RHS to use (in Matrix Market format)')
  parser.add_argument('-p', '--precon',
                      default=False, action='store_true',
                      help='Use preconditioner')
  parser.add_argument('-d', '--directory',
                      help='Path to directory to load vector & matrix from')
  parser.add_argument('-ms', '--maxsize',
                      type=int, help='Maximum system size')
  args = parser.parse_args()

  if args.directory:
    files = os.listdir(args.directory)
    # Check we have a matrix (name.mtx) and a corresponding RHS (name_b.mtx)
    mat = min(files, key=len)
    mat_basename = os.path.basename(mat).replace('.mtx', '')
    vec = mat_basename + '_b.mtx'
    sol = mat_basename + '_x.mtx'
    if os.path.exists(os.path.join(args.directory, sol)):
      print 'Warning! Ignoring provided solution vector', sol
    matrix_path = os.path.join(args.directory, mat)
    vec_path = os.path.join(args.directory, vec)
    if not os.path.exists(matrix_path) or not os.path.exists(vec_path):
      print 'Error expecting at least a matrix and a vector file'
      print 'Check {} and {}'.format(matrix_path, vec_path)
      sys.exit(1)
  else:
    matrix_path = args.matrix_path
    vector_path = args.b

  runCgSolvers(args, matrix_path, vec_path)


if __name__ == '__main__':
  main()
