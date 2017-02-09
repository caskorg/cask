import argparse
import numpy as np
import os
import scipy
import scipy.io
import scipy.sparse
import scipy.sparse.linalg as spla
import sys
import time

from termcolor import colored


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


def residual(matrix, sol, rhs):
    return l2norm(matrix * sol, rhs)


def solve(matrix_path, vec_path=None, sol_path=None, writeToFile=False, checkResidual=False):
    matrixDir = os.path.dirname(os.path.abspath(matrix_path))
    matrixName = os.path.basename(os.path.abspath(matrix_path)).replace('.mtx', '')
    if not vec_path:
        vec_path = os.path.join(matrixDir, matrixName + "_b.mtx")

    system = scipy.sparse.csr_matrix(scipy.io.mmread(matrix_path))
    rhs = scipy.io.mmread(vec_path)
    rhs = np.reshape(rhs, len(rhs))
    if sol_path:
        sol = scipy.io.mmread(sol_path)
        sol = np.reshape(sol, len(sol))
        print 'Solution residual', l2norm(system.dot(sol), rhs)
    res = scipy.sparse.linalg.spsolve(system, rhs)

    if writeToFile:
        file = os.path.join(matrixDir, matrixName + "_x.mtx")
        scipy.io.mmwrite(file, res.reshape(len(res), 1))
    if checkResidual:
        system = scipy.sparse.csr_matrix(scipy.io.mmread(matrix_path))
        got = system.dot(res)
        resNorm = l2norm(rhs, got)
        print 'Residual norm =', resNorm
    return res


def runOnDirectory(dir_path):
    """
    Naming convention:
    1. directory contains several systems: name.mtx, name_b.mtx, name_x.mtx
    2. SPD systems should be named: name_SPD.mtx, name_SPD_b.mtx, name_SPD_x.mtx

    This function will load the systems from the given directory, solve them
    and check against the reference solution, if provided.
    """
    names = set()
    for a in os.listdir(dir_path):
        names.add(a.replace(".mtx", "").replace("_x", "").replace("_b", ""))

    for matrix in names:
        print 'Solving for matrix: ', matrix
        matrix_path=os.path.join(os.path.abspath(dir_path), matrix + ".mtx")
        vec_path=os.path.join(os.path.abspath(dir_path), matrix + "_b.mtx")
        sol_path=os.path.join(os.path.abspath(dir_path), matrix + "_x.mtx")
        if not os.path.exists(sol_path):
            sol_path=None

        solve(matrix_path=matrix_path,
              vec_path=vec_path,
              sol_path=sol_path,
              writeToFile=not sol_path,
              checkResidual=True)


def runSolvers(matrix_path, vec_path, use_precon=True, max_size=None):
    system = scipy.sparse.csr_matrix(scipy.io.mmread(matrix_path))
    rhs = scipy.io.mmread(vec_path)
    n = system.shape[0]
    if max_size and n > max_size:
        print colored('System too large ignoring')
        sys.exit(1)
    print colored('Testing system ' + matrix_path, 'red')
    print scipy.io.mminfo(matrix_path), scipy.io.mminfo(vec_path), float(system.nnz) / n

    resultsList = []
    t0 = time.time()
    dir_sol = scipy.sparse.linalg.spsolve(system, rhs)
    t1 = time.time()
    directSolveTime = t1 - t0
    if type(rhs) is not np.ndarray:
        rhs = rhs.toarray()
    check('Direct Solver', np.squeeze(rhs), system.dot(dir_sol))

    # -- CG Solver
    for solver in [spla.bicgstab]:  # , spla.cg, spla.gmres]: #, spla.bicg, spla.cgs, spla.minres, spla.qmr]:
        # We can try these out in higher ranges, but for now it seems that ideal values
        # are around 100 for the fill_factor and 1e-12 for the drop tollerance
        for fill_factor in [10, 100]:  # 10000, 1000, 100, 10]:
            for drop_tol in [1e-6, 1e-12]:  # , 1e-8, 1e-6]:
                t0 = time.time()
                approx_inv = scipy.sparse.linalg.spilu(
                    scipy.sparse.csc_matrix(system),
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

    headers = ['fill_factor', 'drop_tol', 'solver', 'precon', 'iterations', 'direct(s)', 'precon(s)', 'it(s)',
               'itTotal(s)', 'l2norm']
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
        vec_path = args.b

    runSolvers(args, matrix_path, vec_path)


if __name__ == '__main__':
    main()
