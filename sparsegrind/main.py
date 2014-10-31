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

from math import log, ceil
from reorder import reorder
from sparsegrindio import io


bytes_per_data = 8
bytes_per_metadata = 4


def coo_storage(matrix):
    nnz = matrix.nnz
    return (2 * nnz * bytes_per_metadata,
            nnz * bytes_per_data,
            'COO')


def csc_storage(matrix):
    nnz = matrix.nnz
    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            nnz * bytes_per_data,
            'CSC')


def csr_storage(matrix):
    nnz = matrix.nnz
    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            nnz * bytes_per_data,
            'CSR')


def storage_analysis(matrix):
    fig, ax = pl.subplots()
    width = 0.10

    labels = []
    metadata_bytes = []
    data_bytes = []
    total_bytes = []

    formats = [
        csr_storage(matrix),
        csc_storage(matrix),
        coo_storage(matrix)
    ]

    for f in formats:
        metadata_bytes.append(f[0])
        data_bytes.append(f[1])
        total_bytes.append(f[0] + f[1])
        labels.append(f[2])

    print metadata_bytes
    ind = np.arange(len(formats))
    metadata = ax.bar(ind, metadata_bytes, width, color='r')
    data = ax.bar(ind + width, data_bytes, width, color='y')
    total = ax.bar(ind + 2 * width, total_bytes, width, color='b')
    ax.set_xticks(ind + width)
    ax.set_xticklabels(labels)
    ax.legend((metadata[0], data[0], total[0]),
              ('Metadata (Bytes)', 'Data (Bytes)', 'Total (Bytes)'))
    pl.show()


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


def changes_analysis(matrix_timeline):
    size = len(matrix_timeline[0])
    prev_m = np.zeros((size, size))
    different = {}
    pos = 0
    for m in matrix_timeline:
        if not (m == prev_m).all():
            different[pos] = m
        prev_m = m
        pos += 1
    return different


def reorder_analysis(matrix):
    results = []
    print dir(reorder)
    results.extend(
        [reorder.rcm(matrix),
         reorder.rcm_min_degree(matrix),
         reorder.cm(matrix)]
    )
    return results


def plot_matrices(list_of_matrices):
    """"Plots the given list of sparse matrices using plt.spy()"""
    nplots = len(list_of_matrices)
    for i, m in enumerate(list_of_matrices):
        pl.subplot(nplots, 1, i)
        pl.spy(m)
    pl.show()


def main():

    parser = argparse.ArgumentParser(
        description='Analyse sparse matrices.')
    parser.add_argument('-f', '--format',
                        default='mm',
                        choices=['mm', 'csr', 'coo', 'matlabtl'],
                        help='Format of the given matrix')
    parser.add_argument('-a', '--analysis',
                        default='sparsity',
                        choices=['sparsity', 'range',
                                 'storage', 'changes',
                                 'reordering'],
                        help='Analysis to run')
    parser.add_argument('-t', '--timestep',
                        default=0,
                        help='Time step to look at when using the matlabtl format')
    parser.add_argument('file')
    args = parser.parse_args()
    print args

    timestep = int(args.timestep)
    # read in matrix data
    if args.format == 'matlabtl':
        matrices, realms, imagms = io.read_matlab_matrix_timeline(
            args.file,
            timestep + 1
        )
    elif args.format == 'mm':
        realms = [io.read_matrix_market(args.file)]
    else:
        print 'Unsupported format'
        return

    # perform requested analysis
    if args.analysis == 'sparsity':
        if args.format == 'matlabtl':
            A = sparse.csr_matrix(realms[timestep])
            pl.spy(A)
            pl.show()
    elif args.analysis == 'range':
        A = sparse.csr_matrix(realms[0])
        value_dict = range_analysis(A)
    elif args.analysis == 'changes':
        if args.format != 'matlabtl':
            print 'Changes analysis only supported in matlabtl format.'
            return
        res = changes_analysis(realms)
        res2 = changes_analysis(imagms)
        nitems = len(res) + len(res2)
        for i, k in enumerate(sorted(res.iterkeys())):
            print i
            pl.subplot(nitems, 1, i + 1)
            pl.spy(res.get(k))
        for i, k in enumerate(sorted(res2.iterkeys())):
            pl.subplot(nitems, 1, len(res) + i + 1)
            pl.spy(res2.get(k))
        pl.show()
    elif args.analysis == 'reordering':
        plot_matrices(reorder_analysis(sparse.csr_matrix(realms[0])))
    elif args.analysis == 'storage':
        print 'Running storage format analysis'
        storage_analysis(sparse.csr_matrix(realms[0]))
    else:
        print 'Unspported analysis'
        return

if __name__ == '__main__':
    main()
