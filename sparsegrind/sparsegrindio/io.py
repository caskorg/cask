from scipy import io, sparse
import numpy as np


def count_lines(file_path):
    f = open(file_path)
    c = 0
    for l in f:
        c += 1
    f.close()
    return c


def read_matlab_matrix_timeline(file_path, ntimepoints=None, matrixsize=None):
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
    if not matrixsize:
        matrixsize = count_lines(file_path)

    f = open(file_path)

    firstline = f.readline()
    f.close()
    print len(firstline)
    nvalues = firstline.count(',') + 1

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
    return map(tl_to_csr, [matrices, realms, complexms])


def tl_to_csr(matrix_timeline):
    return [sparse.csr_matrix(m) for m in matrix_timeline]


def read_matrix_market(file_path):
    return sparse.csr_matrix(io.mmread(file_path))


def write_org_table_header(header):
    print '|',
    for h in header:
        print '{0:^20}|'.format(h),
    print ''
    print '|',
    for h in header:
        print '-' * 20 + '+',
    print


def write_org_table_row(row_values):
    print '|',
    for v in row_values:
        if isinstance(v, float):
            print '{0:^20.2f}|'.format(v),
        else:
            print '{0:^20}|'.format(v),
    print
