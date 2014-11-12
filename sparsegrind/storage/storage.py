from scipy.sparse import dia_matrix

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
