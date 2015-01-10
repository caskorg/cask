"""This module is used for the storage analysis."""

from scipy.sparse import dia_matrix

import collections
import math
from math import ceil

bytes_per_data = 8
bytes_per_metadata = 4


def coo(matrix):
    nnz = matrix.nnz
    return (2 * nnz * bytes_per_metadata,
            nnz * bytes_per_data,
            'COO')


def csc(matrix):
    nnz = matrix.nnz
    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            nnz * bytes_per_data,
            'CSC')


def csr(matrix):
    nnz = matrix.nnz
    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            nnz * bytes_per_data,
            'CSR')


def dia(matrix):
    dia_m = dia_matrix(matrix)
    nnz = dia_m.nnz
    return (bytes_per_metadata * len(dia_m.offsets),
            nnz * bytes_per_data,
            'DIA')


def bounded_dictionary(n, matrix_values, k=None):
    """Do a bounded dictionary compression of the given stream of values.
    This works by finding the k highest frequency elements and
    replacing their occurence with a pointer. When k is not specified,
    the encoding includes all elements. """
    counter = collections.Counter()
    for v in matrix_values:
        counter[v] += 1

    nnzs = len(matrix_values)

    # assume all are covered if k is not specified
    covered = float(nnzs)
    bits_per_entry = ceil(math.log(k if k else nnzs, 2))
    if k:
        covered = 0.0
        for v, c in counter.most_common(k):
            covered += c

    bytes_compressed_entries = ceil(covered * bits_per_entry / 8.0)
    bytes_uncompressed_entries = (nnzs - covered) * bytes_per_data
    bytes_overhead = n * bytes_per_metadata if k else 0

    return ceil(bytes_compressed_entries +
                bytes_uncompressed_entries +
                bytes_overhead), counter, covered


def csr_bounded_dictionary(matrix, dict_size=10):
    """Estimate the storage for the given matrix after
    we encode it as CSR and apply bounded dictionary
    encoding to the values stream"""
    nnz = matrix.nnz

    value_bytes = bounded_dictionary(matrix.data, dict_size)

    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            value_bytes,
            'CSR_BD')
