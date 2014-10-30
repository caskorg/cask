"""
A script/module for analysing sparse matrices. Can be used to analyse
1. sparsity
2. dynamic range
3. storage format
"""

import argparse
import numpy as np
import matplotlib.pylab as pl
from scipy import sparse, io
from math import log, ceil


def read_matlab_matrix_timeline(file_path, ntimepoints=None):
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

    if not ntimepoints:
        ntimepoints = nvalues / matrixsize

    matrices = []
    realms = []
    complexms = []
    print ntimepoints
    for t in xrange(ntimepoints):
        matrices.append(np.zeros((matrixsize, matrixsize), dtype='complex'))
        realms.append(np.zeros((matrixsize, matrixsize), dtype='float'))
        complexms.append(np.zeros((matrixsize, matrixsize), dtype='float'))

    f = open(file_path)
    matline = 0
    for line in f:
        #print matline,'/', matrixsize
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


def read_matrix_market(file_path):
    return io.mmread(file_path)


def range_analysis(csr_matrix):
    value_dict = {}
    minCell = None
    maxCell = None
    for d in csr_matrix.data:
        c = value_dict.get(d, 0)
        c += 1
        value_dict[d] = c
        minCell = d if not minCell else min(minCell, d)
        maxCell = d if not minCell else max(maxCell, d)

    print 'Min Value:', minCell
    print 'Max Value:', maxCell
    print 'Range:', maxCell - minCell

    prec = [1E-3, 1E-6, 1E-9, 1E-12]
    for p in prec:
        print '{} bits to represent with {} precision'.format(
            int(ceil(log((maxCell - minCell)/p, 2))), p)


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
        matrices, realms, imagms = read_matlab_matrix_timeline(args.file, 1)
    elif args.format == 'mm':
        realms = [read_matrix_market(args.file)]
    else:
        print 'Unsupported format'
        return

    # perform requested analysis
    if args.analysis == 'sparsity':
        A = sparse.csr_matrix(realms[0])
        pl.spy(A)
        pl.show()
    elif args.analysis == 'range':
        A = sparse.csr_matrix(realms[0])
        value_dict = range_analysis(A)

    else:
        print 'Unspported analysis'
        return

if __name__ == '__main__':
    main()
