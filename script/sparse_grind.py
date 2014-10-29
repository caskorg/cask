"""
A script/module for analysing sparse matrices. Can be used to analyse
1. sparsity
2. dynamic range
3. storage format
"""

import argparse
import numpy as np
import matplotlib.pylab as pl
from scipy import sparse


def read_matlab_matrix_timeline(file_path):
    """Reads a timeline of complex valued matrices produced from Matlab.
    Matrices are stored sequentially, in dense format with values
    separated by commas e.g.
      (A)     (B)     (C)
    a11, a12, b11, b12, c11, c12
    a21, a22, b21, b22, c21, c22

    Args:
      file_path - path to the matrix file.

    Returns:
      A list of tuples [(complex, real, imag)] representing the
    complex, real and imaginary _dense_ matrices corresponding to the
    input data.
    """
    f = open(file_path)
    matrixsize = 68

    firstline = f.readline()
    f.close()
    print len(firstline)
    nvalues = firstline.count(',')

    ntimepoints = nvalues / matrixsize

    matrices = []
    realms = []
    complexms = []
    print ntimepoints
    for t in xrange(ntimepoints):
        matrices.append(np.zeros((matrixsize, matrixsize), dtype='complex'))
        realms.append(np.zeros((matrixsize, matrixsize), dtype='complex'))
        complexms.append(np.zeros((matrixsize, matrixsize), dtype='complex'))

    f = open(file_path)
    matline = 0
    for line in f:
        print matline,'/', matrixsize
        values = line.split(',')
        for t in xrange(ntimepoints):
            for i in xrange(matrixsize):
                idx = t * matrixsize + i
                cvalue = complex(values[idx].strip().replace('i', 'j'))
                matrices[t][matline, i] = cvalue
                realms[t][matline, i] = cvalue.real
                complexms[t][matline, i] = cvalue.imag
        matline += 1
    f.close()
    return matrices, realms, complexms


def main():

    parser = argparse.ArgumentParser(
        description='Analyse sparse matrices.')
    parser.add_argument('-f', '--format',
                        default='mm',
                        choices=['mm', 'csr', 'coo', 'matlabtl'],
                        help='Format of the given matrix')
    parser.add_argument('-a', '--analysis',
                        default='sparsity',
                        choices=['sparsity', 'range', 'storage'],
                        help='Analysis to run')
    parser.add_argument('file')
    args = parser.parse_args()
    print args

    # read in matrix data
    if args.format == 'matlabtl':
        matrices, realms, imagms = read_matlab_matrix_timeline(args.file)
    else:
        print 'Unsupported format'

    # perform requested analysis
    if args.analysis == 'sparsity':
        A = sparse.csr_matrix(realms[0])
        pl.spy(A)
        pl.show()
    else:
        print 'Unspported analysis'

if __name__ == '__main__':
    main()
