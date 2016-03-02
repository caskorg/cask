import argparse
import scipy
from scipy import io, sparse
from scipy.sparse import linalg
import numpy as np
import os
import sys

import scipy.sparse.linalg as spla

def runSolver(solver, system, rhs, squeeze=True):
  sol = solver(system, rhs)
  got = rhs
  if squeeze:
    got = np.squeeze(rhs.toarray())
  check = system.dot(sol).transpose()
  print np.linalg.norm(np.subtract(got, check))

def check(msg, exp, got):
  print msg, np.linalg.norm(np.subtract(got, exp))


def main():
  parser = argparse.ArgumentParser(
      description='Benchmark sparse linear solvers',
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('-m', '--matrix-path',
      help='Path to matrix to use (in Matrix Market Format')
  parser.add_argument('-b',
      help='Path to RHS to use (in Matrix Market format)')
  parser.add_argument('-d', '--directory',
      help='Path to directory to load vector & matrix from')
  args = parser.parse_args()

  if args.directory:
    files = os.listdir(args.directory)
    # Check we have a matrix (name.mtx) and a corresponding RHS (name_b.mtx)
    if len(files) != 2:
      print 'Error! Expecting exactly 2 files'
      sys.exit(1)
    if len(files[0]) < len(files[1]):
      mat, vec = files[0], files[1]
    else:
      mat, vec = files[1], files[0]

    mat_basename = os.path.basename(mat).replace('.mtx', '')
    vec_basename = os.path.basename(vec).replace('.mtx', '')

    if mat_basename + "_b" != vec_basename:
      print 'Error! Matrix, vector should be named: matrix.mtx, matrix_b.mtx'
      sys.exit(1)

    matrix_path = os.path.join(args.directory, mat)
    vec_path = os.path.join(args.directory, vec)
  else:
    matrix_path = args.matrix_path
    vector_path = args.b

  system = sparse.csr_matrix(io.mmread(matrix_path))
  rhs = io.mmread(vec_path)

  # -- Direct solver
  dir_sol = scipy.sparse.linalg.spsolve(system, rhs)
  check('Direct Solver', np.squeeze(rhs.toarray()), system.dot(dir_sol))

  # -- CG Solver
  for solver in [spla.bicgstab, spla.cg, spla.gmres, spla.bicg, spla.cgs, spla.minres, spla.qmr]:
    for fill_factor in [10]: # 10000, 1000, 100, 10]:
      for drop_tol in [1e-12, 1e-8, 1e-6]:
        approx_inv = scipy.sparse.linalg.spilu(
            sparse.csc_matrix(system),
            fill_factor = fill_factor,
            drop_tol = drop_tol
            )
        n = system.shape[0]
        # print approx_inv
        M = scipy.sparse.linalg.LinearOperator(
            (n, n), lambda x: approx_inv.solve(x))

        sol = solver(system, rhs.toarray(), M=M, maxiter=n)
        result = sol[0]
        # print 'Direct solution', dir_sol
        # print 'Iterative', result
        check('fill_factor {:4} drop_top {:4} solver {:10}'.format(
          fill_factor, drop_tol, solver),
          np.squeeze(rhs.toarray()), system.dot(result))

if __name__ == '__main__':
  main()
