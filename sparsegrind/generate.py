import numpy as np
import random
import scipy
import sys
import scipy.sparse
import scipy.linalg
import scipy.io
import grindlinalg


def generate_system(n, ratio, spd=False, maxTries=10, name="tmp"):
    if name:
        lhs_name = name + "_x.mtx"
        rhs_name = name + "_b.mtx"
        matrix_name = name + ".mtx"

    A = generate_matrix(n, ratio, spd, maxTries, file=matrix_name)
    rhs = generate_vec(n, value=5.5, file=rhs_name)
    lhs = grindlinalg.solve(matrix_path=matrix_name, vec_path=rhs_name, writeToFile=True)
    return A, lhs, rhs


def generate_matrix(n, ratio, spd=False, maxTries=10, file=None):
    if n > 1000:
        print 'Warning! Routine not optimised for large matrices'

    nonSingular = True

    A = None
    count = 0
    while nonSingular and count < maxTries:
        nonSingular = False
        try:
            A = scipy.sparse.csr_matrix(scipy.sparse.rand(n, n, ratio))
            invA = scipy.linalg.inv(A.todense())
            print 'Nonzeros in inverse: ', len(invA.nonzero())
        except np.linalg.LinAlgError as e:
            nonSingular = True
            print 'Warning, error occurred generating matrix - not singular; giving it another shot'
            print e
            count += 1

    if count == maxTries:
        raise RuntimeError('Could not generate matrix')

    res = A
    if spd:
        # A nonsingular ==> A.T * A is positive definite
        # https://en.wikipedia.org/wiki/Positive-definite_matrix
        # Also, A.T * A is symmetric
        res = A.transpose() * A

    if  file:
        scipy.io.mmwrite(file, res, field='real')
    return res


def generate_vec(n, value=0.0, file=None):
    res = np.array([float(value)] * n)
    if file:
        scipy.io.mmwrite(file, res.reshape(n, 1))
    return res


def print_header():
    print '%%MatrixMarket matrix coordinate real general'


def main():
    if len(sys.argv) < 3:
        print 'usage: generate_matrices rows cols elements_per_row'
        sys.exit(1)

    rows = int(sys.argv[1])
    cols = int(sys.argv[2])
    elements_per_row = int(sys.argv[3])

    print_header()
    print rows, cols, rows * elements_per_row

    for i in range(1, rows + 1):
        for j in range(1, elements_per_row + 1):
            value = random.random()
            print i, j, value


if __name__ == '__main__':
    main()
