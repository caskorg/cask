import sparsegrind
import sparsegrind.generate
from sparsegrind import matrixio
from sparsegrind import storage
from sparsegrind import main
from sparsegrind import grindlinalg

import os
import unittest
import numpy as np
from numpy import testing


def path_to(file):
    return os.path.join(os.path.dirname(__file__), file)


def remove(file):
    try:
        os.remove(file)
    except OSError as e:
        print 'Warning, issue removing file', e


class TestMain(unittest.TestCase):

    def setUp(self):
        self.csr_matrix = matrixio.read_matrix_market(path_to("small.mtx"))
        self.timeline = matrixio.read_matlab_matrix_timeline(
            path_to("small.matlabtl"))

    def tearDown(self):
        remove("test.mtx")
        remove("test_b.mtx")

    def testCountLines(self):
        self.assertEquals(
            matrixio.count_lines(path_to("small.matlabtl")),
            2)

    def testCsrRead(self):
        self.assertEquals(len(self.csr_matrix.data), 8)
        self.assertEquals(len(self.csr_matrix.indptr), 5 + 1)
        testing.assert_allclose(
            self.csr_matrix.data,
            np.array([1, 6, 10.5, 0.015, 250.5, -280, 33.32, 10.5]))

    def testTimelineRead(self):
        cmplx = self.timeline[0]
        self.assertEquals(len(cmplx), 2)
        testing.assert_allclose(
            cmplx[0].data,
            np.array([2+1j, 1+3j, 3, 4]))
        testing.assert_allclose(
            cmplx[1].data,
            np.array([2+1j, 1.1+3j, 3.1]))

    def testStorage(self):
        self.assertEquals(
            storage.coo(self.csr_matrix),
            (64, 64, 'COO'))

        self.assertEquals(
            storage.csr(self.csr_matrix),
            (4 * (8 + 6), 64, 'CSR 64 bits data 32 bit index'))

        self.assertEquals(
            storage.csc(self.csr_matrix),
            (4 * (8 + 6), 64, 'CSC'))

    def testChangesAnalysis(self):
        result = main.changes_analysis(self.timeline[0])
        self.assertEquals(len(result), 2)
        testing.assert_allclose(result[0].data,
                                np.array([2+1j, 1+3j, 3, 4]))
        testing.assert_allclose(result[1].data,
                                np.array([2+1j, 1.1+3j, 3.1]))

    def testRangeAnalysis(self):
        minCell, maxCell, unique = main.range_analysis(self.csr_matrix)
        self.assertEquals(-280, minCell)
        self.assertEquals(250.5, maxCell)
        self.assertEquals(7, unique)

    def testLinalg(self):
        size = 5
        A = sparsegrind.generate.generate_matrix(size, 0.5)
        self.assertEquals(A.shape[0], size)
        self.assertEquals(A.shape[1], size)

        A = sparsegrind.generate.generate_matrix(size, 0.5, spd=True)
        self.assertEquals(A.shape[0], size)
        self.assertEquals(A.shape[1], size)

        A = sparsegrind.generate.generate_matrix(size, 0.5, spd=True, file="test.mtx")
        self.assertEquals(A.shape[0], size)
        self.assertEquals(A.shape[1], size)

        vec = sparsegrind.generate.generate_vec(A.shape[1], 5, "test_b.mtx")
        sol = grindlinalg.solve(matrix_path="test.mtx", vec_path="test_b.mtx")
        res = grindlinalg.residual(A, sol, vec)
        self.assertAlmostEqual(res, 0)

        print '==> Matrix\n {}\n==> Lhs\n {}\n==> Rhs\n {}\n==> Residual\n {}'.format(
           A.todense(), sol, vec, res
        )

        grindlinalg.runOnDirectory('test/systems/')


if __name__ == '__main__':
    unittest.main()
