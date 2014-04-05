# Checks that a given solution to a system is correct (within some
# tolerance). To keep things simple (and correct), this will perform
# all operations as dense.

# Usage: check_sol.py <matrix>xx
# Expects to find
#    1. matrix.mtx (the system matrix)
#    2. matrix_sol.mtx (the generated solution)
#    3. matrix_b.mtx (the RHS of the system)

# Tolerance
EPSILON = 1e-2

import numpy as np
import sys

system = sys.argv[1]

f = open(system + '.mtx')

lineNumber = 0

symmetric = False
coordinate = False
real = False
first = True
mat = None

def readArray(f):
    lineNumber = 0
    first = True
    i = 0
    # read RHS as dense array
    for line in f:

        if lineNumber == 0:
            # parse header
            array = line.find('array') != -1
        lineNumber += 1
        line = line.strip()
        if line[0] == '%':
            # skip header stuff
            continue

        dims = line.split(' ')
        if first:
            n = int(dims[0])
            rhs = np.zeros((n, ))
            first = False
            continue

        # assuming array format
        rhs[i] = float(dims[0])
        i += 1
    return i, array, rhs


# read input matrix
for line in f:

    if lineNumber == 0:
        # parse header
        symmetric = line.find('symmetric') != -1
        coordinate = line.find('coordinate') != -1
        real = line.find('real') != -1
    lineNumber += 1

    if line[0] == '%':
        # skip header stuff
        continue

    dims = line.split(' ')
    if first:
        n = int(dims[0])
        m = int(dims[1])
        l = int(dims[2])

        mat = np.zeros((n, n))
        first = False
        continue

    # assuming COO format
    x, y = int(dims[0]) - 1, int(dims[1]) - 1
    mat[x, y] = dims[2]
    mat[y, x] = dims[2]

print 'Read matrix', n, m, l, symmetric, coordinate, real
# print mat

f.close()

f = open(system + '_b.mtx')
nrhs, arrayrhs, rhs = readArray(f)
f.close()
print 'Read RHS', nrhs, arrayrhs
# print rhs

f = open(system + '_sol.mtx')
nx, arrayx, x = readArray(f)
f.close()
print 'Read Sol', nx, arrayx
# print x

print 'Computed rhs'
computed_rhs = mat.dot(x)

for i in range(nx):
    if abs(computed_rhs[i] - rhs[i]) > 1e-5:
        print 'Error:', i, 'expected', '{0:.12f}'.format(rhs[i]), 'got', computed_rhs[i]
