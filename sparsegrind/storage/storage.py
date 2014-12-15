"""This module is used for the storage analysis."""

from scipy.sparse import dia_matrix

import collections
import math

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


def bounded_dictionary(matrix_values, k):
    """Do a bounded dictionary compression of the given stream of values.
    This works by finding the k highest frequency elements and
    replacing their occurence with a pointer."""
    counter = collections.Counter()
    for v in matrix_values:
        counter[v] += 1
        
    covered = 0.0
    for v, c in counter.most_common(k):
        covered += c

    return math.ceil(covered * math.ceil(math.log(k, 2)) + 
                     (len(matrix_values) - covered) * bytes_per_data + 
                     len(matrix_values) ) / 8
    

def csr_bounded_dictionary(matrix, dict_size=10):
    """Estimate the storage for the given matrix after 
    we encode it as CSR and apply bounded dictionary 
    encoding to the values stream"""
    nnz = matrix.nnz

    value_bytes = bounded_dictionary(matrix.data, dict_size)

    return ((len(matrix.indptr) + nnz) * bytes_per_metadata,
            value_bytes,
            'CSR_BD')
