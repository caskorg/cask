from sparsegrind.sparsegrindio import io
import os
import unittest


def path_to(file):
    return os.path.join(os.path.dirname(__file__), file)


class TestMain(unittest.TestCase):

    def setUp(self):
        self.csr_matrix = io.read_matrix_market(path_to("small.in"))

    def test_main(self):
        self.assertEquals(len(self.csr_matrix.data), 8)
        self.assertEquals(len(self.csr_matrix.indptr), 5 + 1)


if __name__ == '__main__':
    unittest.main()
