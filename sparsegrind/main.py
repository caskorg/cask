"""
A module for analysing sparse matrices. Can be used to analyse:
1. sparsity pattern
2. dynamic range
3. storage format
"""

import argparse
import numpy as np
import matplotlib.pylab as pl
import os
import os.path

from math import log, ceil
from reorder import reorder
from sparsegrindio import io
from storage import storage
import os


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
        storage.csr_bounded_dictionary(matrix, 64),
        # storage.csr_bounded_dictionary(matrix, 10000),
    ]

    # storage.dia(matrix),

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

    return formats


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


def write_org_table_row(row_values):
    print row_values


def write_org_table_header(header):
    print header


def compression_analysis(matrix, name):
    results = [name]
    results.append('?')
    results.append(matrix.nnz)
    results.append(len(matrix.indptr))
    results.append(len(set(matrix.data)))

    bcsr = storage.csr_bounded_dictionary(matrix, 128)
    csr = storage.csr(matrix)
    total = (csr[0] + csr[1]) / (bcsr[0] + bcsr[1])
    metadata = csr[0] / bcsr[0]
    value = csr[1] / bcsr[1]
    results.append(value)
    results.append(metadata)
    results.append(total)

    write_org_table_row(results)


def plot_matrices(list_of_matrices):
    """"Plots the given list of sparse matrices using plt.spy()"""
    nplots = len(list_of_matrices)
    for i, m in enumerate(list_of_matrices):
        pl.subplot(nplots, 1, i + 1)
        pl.spy(m)
    pl.show()


def grind_matrix(file, args):

    name = os.path.basename(file).replace('.mtx', '')

    # read in matrix data
    if args.format == 'matlabtl':
        matrices, realms, imagms = io.read_matlab_matrix_timeline(
            file,
            args.timestep + 1
        )
    elif args.format == 'mm':
        realms = [io.read_matrix_market(file)]
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
    elif args.analysis == 'compression':
        compression_analysis(realms[0], name)
    else:
        print 'Unspported analysis'
        return


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
                                 'reordering',
                                 'compression'],
                        help='Analysis to run')
    parser.add_argument('-t', '--timestep',
                        default=0,
                        type=int,
                        help='Time step when using the matlabtl format')
    parser.add_argument('-r', '--recursive',
                        action='store_true',
                        help='Recursive. Only for .mtx files.')
    parser.add_argument('file')
    args = parser.parse_args()

    if args.recursive:
        if args.analysis == 'compression':
            header = ['name', 'sym', 'nnzs', 'dim', 'uvals',
                      'B_CSR(values)', 'B_CSR(meta)', 'B_CSR(total)']
            write_org_table_header(header)
        parent_dir = os.path.dirname(os.path.abspath(args.file))
        for root, dirs, files in os.walk(parent_dir):
            for f in files:
                if f.endswith('.mtx'):
                    grind_matrix(os.path.join(root, f), args)
        return

    grind_matrix(args.file, args)

if __name__ == '__main__':
    main()
