import scipy
import scipy.io
import numpy as np

import sys


def main():
    path = sys.argv[1]

    A = scipy.io.mmread(path)

    v = None

    if len(sys.argv) <= 2:
        # generate vector
        v = np.array(range(A.shape[1]))
    else:
        # untested
        v = scipy.io.mmread(sys.argv[2])

    scipy.io.mmwrite("test_small_v.mtx", v)
    print A.dot(v)

if __name__ == '__main__':
    main()
