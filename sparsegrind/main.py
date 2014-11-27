"""
A module for analysing sparse matrices. Can be used to analyse:
1. sparsity pattern
2. dynamic range
3. storage format
"""

import argparse
import numpy as np
import matplotlib.pylab as pl

from math import log, ceil
from reorder import reorder
from sparsegrindio import io
from storage import storage


def storage_analysis(matrix):
    """Plots the storage cost in bytes of various formats."""
    fig, ax = pl.subplots()
    width = 0.10

    labels = []
    metadata_bytes = []
    data_bytes = []
    total_bytes = []

    formats = [
        storage.csr(matrix),
        storage.csc(matrix),
        storage.coo(matrix),
        storage.dia(matrix),
    ]

    for f in formats:
        metadata_bytes.append(f[0])
        data_bytes.append(f[1])
        total_bytes.append(f[0] + f[1])
        labels.append(f[2])

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
    """Analyse the range of values and the number of bits required to
    represent it (as fixed point) for the given matrix."""
    value_dict = {}
    minCell = None
    maxCell = None
    for d in csr_matrix.data:
        c = value_dict.get(d, 0)
        c += 1
        value_dict[d] = c
        minCell = d if not minCell else min(minCell, d)
        maxCell = d if not minCell else max(maxCell, d)

    prec = [1E-3, 1E-6, 1E-9, 1E-12]
    for p in prec:
        print '{} bits to represent with {} precision'.format(
            long(ceil(log((maxCell - minCell)/p, 2))), p)

    sorted_values = sorted(value_dict.values(), reverse=True)
    print 'Highest frequencies: ', sorted_values[0:10]

    return minCell, maxCell, len(value_dict.keys())


def changes_analysis(matrix_timeline):
    """Identify the points in the timeline where the matrix changes with
    respect to previous values."""
    different = {}
    different[0] = prev_m = matrix_timeline[0]
    pos = 1
    for m in matrix_timeline[1:]:
        if (m != prev_m).data.any():
            different[pos] = m
        prev_m = m
        pos += 1
    return different


def reorder_analysis(matrix):
    """Returns the results of applying various reordering algorithms to
    the given matrix."""
    return [reorder.rcm(matrix),
            reorder.rcm_min_degree(matrix),
            reorder.cm(matrix)]


def plot_matrices(list_of_matrices):
    """"Plots the given list of sparse matrices using plt.spy()"""
    nplots = len(list_of_matrices)
    for i, m in enumerate(list_of_matrices):
        pl.subplot(nplots, 1, i + 1)
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
                        type=int,
                        help='Time step to look at when using the matlabtl format')
    parser.add_argument('file')
    args = parser.parse_args()

    # read in matrix data
    if args.format == 'matlabtl':
        matrices, realms, imagms = io.read_matlab_matrix_timeline(
            args.file,
            args.timestep + 1
        )
    elif args.format == 'mm':
        realms = [io.read_matrix_market(args.file)]
    else:
        print 'Unsupported format'
        return

    # perform requested analysis
    if args.analysis == 'sparsity':
        step = args.timestep if args.format == 'matlabtl' else 0
        pl.spy(realms[step])
        pl.show()
    elif args.analysis == 'range':
        minCell, maxCell, uniqueValues = range_analysis(realms[0])
        print 'Min Value:', minCell
        print 'Max Value:', maxCell
        print 'Unique values / total nonzero values:',
        print uniqueValues, ' / ', realms[0].nnz
        print 'Range:', maxCell - minCell
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
        plot_matrices([realms[0]] + reorder_analysis(realms[0]))
    elif args.analysis == 'storage':
        print 'Running storage format analysis'
        storage_analysis(realms[0])
    else:
        print 'Unspported analysis'
        return

if __name__ == '__main__':
    main()
