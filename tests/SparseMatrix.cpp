#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SpamSparseMatrix.hpp>
#include <SpamUtils.hpp>
#include <gtest/gtest.h>

class TestSparseMatrix : public ::testing::Test { };

TEST_F(TestSparseMatrix, DokSetFromPattern) {

   spam::DokMatrix dkMatrix{
       1, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };

   ASSERT_EQ(dkMatrix.nnzs, 10);
   ASSERT_EQ(dkMatrix.n, 4);
   // test some entries
   ASSERT_EQ(dkMatrix.at(0, 0), 1);
   ASSERT_EQ(dkMatrix.at(1, 1), 1);
   ASSERT_EQ(dkMatrix.at(0, 1), 1);
   ASSERT_EQ(dkMatrix.at(0, 2), 1);
   ASSERT_EQ(dkMatrix.at(0, 3), 1);
   ASSERT_EQ(dkMatrix.at(3, 3), 1);
}

TEST_F(TestSparseMatrix, DokExplicitSymmetry) {
   spam::DokMatrix dkMatrix{
       1, 0, 0, 0,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };

   ASSERT_EQ(dkMatrix.nnzs, 7);
   ASSERT_EQ(dkMatrix.n, 4);

   spam::DokMatrix sym = dkMatrix.explicitSymmetric();
   ASSERT_EQ(sym.nnzs, 10);
   ASSERT_EQ(sym.n, 4);

   spam::DokMatrix expSym{
       1, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };
   ASSERT_EQ(sym, expSym);
}

TEST_F(TestSparseMatrix, CsrToFromDok) {

  spam::DokMatrix dokA{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
  };

  spam::CsrMatrix a{dokA};
  ASSERT_EQ(a.toDok().dok, dokA.dok);
  ASSERT_EQ(a.toDok(), dokA);
}


TEST_F(TestSparseMatrix, CsrLowerTriangular) {
  spam::CsrMatrix m{spam::DokMatrix{
      1, 1, 1, 1,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 0, 1
  }};

  std::cout << "Matrix" << std::endl;
  m.pretty_print();

  spam::CsrMatrix expL{spam::DokMatrix{
      1, 0, 0, 0,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 0, 1
  }};

  std::cout << "Lower triangular" << std::endl;
  auto l = m.getLowerTriangular();
  l.pretty_print();

  ASSERT_EQ(l, expL);
}

TEST_F(TestSparseMatrix, CsrUpperTriangular) {
  spam::CsrMatrix m{spam::DokMatrix{
      1, 1, 1, 1,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 0, 1
  }};

  std::cout << "Matrix" << std::endl;
  m.pretty_print();

  spam::CsrMatrix expU{spam::DokMatrix{
      1, 1, 1, 1,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
  }};

  std::cout << "Upper triangular" << std::endl;
  auto u = m.getUpperTriangular();
  u.pretty_print();

  ASSERT_EQ(u, expU);
}
