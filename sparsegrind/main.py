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
import collections

from math import log, ceil
from reorder import reorder
from sparsegrindio import io
from storage import storage
from precision import precision
import os
import sparsegrindio
import sys


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


def compression_analysis_bcsrvi(matrix, name):
    """Prints compression results for BCSRVI normalized wrt CSR and CSRVI."""
    results = [name]
    sh = matrix.shape
    if sh[0] <= 1 or sh[1] <= 1:
        return
    results.append('?')
    results.append(matrix.nnz)
    results.append(len(matrix.indptr))

    # First find reference CSR and CSRVI values
    csr = storage.csr(matrix)
    csr_values = csr[1]
    csr_total = csr[0] + csr[1]

    n = len(matrix.indptr)

    bcsrv_reference_values = storage.bounded_dictionary(n, matrix.data)[0]

    counter = collections.Counter()
    for v in matrix.data:
        counter[v] += 1

    print name,
    for decoding_table_bitwidth in range(1, 17):
        bcsr = storage.bounded_dictionary(n, matrix.data,
                                          decoding_table_bitwidth, counter)[0]
        bcsrv_total = bcsr + csr[0]
        print "{:2f} {:2f} {:2f}".format(csr_values / bcsr,
                                         csr_total / bcsrv_total,
                                         bcsrv_reference_values / bcsr),
    print

def compression_analysis_precision(matrix, name, tolerance):
    """Evenly reduce precision of matrix entries and check if it is still fine for iterative method"""
    results = [name]
    sh = matrix.shape
    if sh[0] <= 1 or sh[1] <= 1:
        return
    results.append('?')
    results.append(matrix.nnz)
    results.append(len(matrix.indptr))

    # First find reference CSR
    csr = storage.csr(matrix)
    csr_values = csr[1]
    csr_total = csr[0] + csr[1]
    norm_orig     = precision.matrix_norms(matrix)
    status, orig_iterations, sol_orig = precision.solve_cg(matrix, tolerance)

    n = len(matrix.indptr)

    if status != 0: print "This matrix, as given, did not ever converge"; return

    # Total # of flops over num iterations
    orig_flop_count = matrix.nnz * orig_iterations
    # Total traffic: fetching matrix, vectors x and y and a diagonal
    # preconditioner @ each iteration.
    orig_traffic = (csr_total + 3*n) * orig_iterations

    np.set_printoptions(precision=2)
    print name,"\n"
    for mantissa_bitwidth in (8,16,20,24,32):
      print "| mantissa {:2d} bits:".format(mantissa_bitwidth),

      reduced_matrix, error = precision.reduce_elementwise(n, matrix, mantissa_bitwidth)

      # analyse precision loss
      status, iterations, sol_reduced = precision.solve_cg(reduced_matrix, tolerance)
      # ignore this precision if not even converged at all
      if status != 0: print "did not converge"; continue
      # did we ever converge to a sensible solution?
      l2error       = precision.l2_error(sol_orig, sol_reduced)
      # ignore this precision if converged to something completely different
      # let's accept 10 times worse accuracy for reduced precision
      if l2error > 10*tolerance: print "converged to a wrong solution"; continue

      # this should stand for poor man estimation of reduction consequences,
      # as _substitute_ to actually solving matrix problem. Matrix norms should
      # be indicative to convergence rates.
      norm_reduced  = precision.matrix_norms(reduced_matrix)

      additional_iterations = iterations - orig_iterations

      print "iterations {:2d} {:2d} {:2d}, l1 norm {:2f} {:2f} {:2f} ".format(
                                       orig_iterations, iterations, additional_iterations,
                                       norm_orig[0], norm_reduced[0],
                                       norm_orig[0]-norm_reduced[0],)

      csr_custom = storage.csr(matrix, mantissa_bitwidth=mantissa_bitwidth, index_bitwidth=np.array([16,32]))
      reduced_total_storage = csr_custom[0] + csr_custom[1]
      value_compression_rate = csr_values/csr_custom[1]
      total_compression_rate = csr_total/reduced_total_storage

      # OK, we reduced precision and got more iterations. Did we win?
      reduced_flop_count = matrix.nnz * iterations
      # Assuming only matrix precision is subject to reduction.
      reduced_traffic = (reduced_total_storage + 3*n) * iterations

      flops_improvement = (orig_flop_count).astype(np.float64)/reduced_flop_count
      traffic_improvement = (orig_traffic).astype(np.float64)/reduced_traffic

      print "  compression: values {:2f}, total ".format(value_compression_rate),
      for c in total_compression_rate:
        print "{:2f}".format(c),
      print "| outcome: flops ratio {:2f}, traffic ratio".format(flops_improvement),
      for t in traffic_improvement:
        print "{:2f}".format(t),
      print

    print


def plot_matrices(list_of_matrices):
    """Plots the given list of sparse matrices using plt.spy()"""
    nplots = len(list_of_matrices)
    for i, m in enumerate(list_of_matrices):
        pl.subplot(nplots, 1, i + 1)
        pl.spy(m)
    pl.show()


def summary_analysis(matrix, name):
    """Prints generic information about the provided sparse matrix."""
    print "Name, Nonzeros, Unique Values, Sparsity, Ratio Uniques"
    uniques = len(set(matrix.data))
    print name, matrix.nnz, uniques,
    print "{:.2f}".format(float(matrix.nnz) / len(matrix.indptr)**2),
    print "{:.2f}".format(uniques / float(matrix.nnz)),


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
    elif args.analysis == 'compress_bcsrvi':
        compression_analysis_bcsrvi(realms[0], name)
    elif args.analysis == 'reduce_precision':
        compression_analysis_precision(realms[0], name, args.tolerance)
    elif args.analysis == 'summary':
        summary_analysis(realms[0], name)
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
                                 'compress_bcsrvi',
                                 'reduce_precision',
                                 'summary'],
                        help='Analysis to run')
    parser.add_argument('-t', '--timestep',
                        default=0,
                        type=int,
                        help='Time step when using the matlabtl format')
    parser.add_argument('-e', '--tolerance',
                        default=1e-5,
                        type=float,
                        help='Acceptable tolerance in floating point computes')
    parser.add_argument('-r', '--recursive',
                        action='store_true',
                        help='Recursive. Only for .mtx files.')
    parser.add_argument('file')
    args = parser.parse_args()

    if args.recursive:
        if args.analysis == 'compression':
            header = ['name', 'sym', 'nnzs', 'dim', 'uvals %',
                      'B_CSR128(values)', 'covered', 'B_CSR128(total)']
            # sparsegrindio.io.write_org_table_header(header)
        parent_dir = os.path.dirname(os.path.abspath(args.file))

        for root, dirs, files in os.walk(parent_dir):
            for f in files:
                if f.endswith('.mtx'):
                    grind_matrix(os.path.join(root, f), args)
        return

    grind_matrix(args.file, args)

if __name__ == '__main__':
    main()
