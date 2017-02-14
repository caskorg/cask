#include <SparseMatrix.hpp>
#include <gtest/gtest.h>
#include <vector>

class TestSparseMatrix : public ::testing::Test { };
class TestVector : public ::testing::Test { };

TEST_F(TestSparseMatrix, DokSetFromPattern) {

   cask::DokMatrix dkMatrix{
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

TEST(TestDokMatrix, SetFromPatternWithGivenRows) {
  cask::DokMatrix dkMatrix{
      1, {4, 5, 3, 2}};
  EXPECT_EQ(dkMatrix.nnzs, 4);
  EXPECT_EQ(dkMatrix.n, 1);
  EXPECT_EQ(dkMatrix.at(0, 0), 4);
  EXPECT_EQ(dkMatrix.at(0, 1), 5);
  EXPECT_EQ(dkMatrix.at(0, 2), 3);
  EXPECT_EQ(dkMatrix.at(0, 3), 2);
}

TEST_F(TestSparseMatrix, DokExplicitSymmetry) {
   cask::DokMatrix dkMatrix{
       1, 0, 0, 0,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };

   ASSERT_EQ(dkMatrix.nnzs, 7);
   ASSERT_EQ(dkMatrix.n, 4);

   cask::DokMatrix sym = dkMatrix.explicitSymmetric();
   ASSERT_EQ(sym.nnzs, 10);
   ASSERT_EQ(sym.n, 4);

   cask::DokMatrix expSym{
       1, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };
   ASSERT_EQ(sym, expSym);
}

TEST_F(TestSparseMatrix, DokDotProduct) {
   cask::DokMatrix dkMatrix{
       1, 0, 0, 0,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   };
   std::vector<double> b{1, 2, 3, 4};
   std::vector<double> exp{1, 3, 4, 5};
   ASSERT_EQ(dkMatrix.dot(b), exp);
}

TEST_F(TestSparseMatrix, CsrToFromDok) {
  cask::DokMatrix dokA{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
  };

  cask::CsrMatrix a{dokA};
  ASSERT_EQ(a.toDok().dok, dokA.dok);
  ASSERT_EQ(a.toDok(), dokA);
}


TEST_F(TestSparseMatrix, CsrLowerTriangular) {
  cask::CsrMatrix m{cask::DokMatrix{
      1, 1, 1, 1,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 0, 1
  }};

  std::cout << "Matrix" << std::endl;
  m.pretty_print();

  cask::CsrMatrix expL{cask::DokMatrix{
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
  cask::CsrMatrix m{cask::DokMatrix{
      1, 1, 1, 1,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 0, 1
  }};

  std::cout << "Matrix" << std::endl;
  m.pretty_print();

  cask::CsrMatrix expU{cask::DokMatrix{
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

TEST(CsrMatrix, RowSlice) {
  cask::CsrMatrix a{cask::DokMatrix{
      1, 2, 5, 4,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
  }};
  cask::CsrMatrix expU(1, {1, 2, 5, 4});
  auto u = a.sliceRows(0, 1);
  EXPECT_EQ(u.values, expU.values);
  EXPECT_EQ(u.row_ptr, expU.row_ptr);
  EXPECT_EQ(u.col_ind, expU.col_ind);

  cask::CsrMatrix expU2{2, {0, 1, 0, 0,
                            0, 0, 1, 0}};
  auto u2 = a.sliceRows(1, 2);
  EXPECT_EQ(u2.values, expU2.values);
  EXPECT_EQ(u2.row_ptr, expU2.row_ptr);
  EXPECT_EQ(u2.col_ind, expU2.col_ind);

  auto u3 = a.sliceRows(0, 4);
  EXPECT_EQ(a, u3);
}


TEST_F(TestSparseMatrix, SymCsrDotProduct) {
  cask::SymCsrMatrix m{cask::DokMatrix{
      1, 0, 0, 0,
      1, 1, 0, 0,
      1, 0, 1, 0,
      1, 0, 1, 1
  }};
  // -- actual matrix:
  // 1, 1, 1, 1,
  // 1, 1, 0, 0,
  // 1, 0, 1, 1,
  // 1, 0, 1, 1
  std::vector<double> b{1, 2, 3, 4};
  std::vector<double> e{10, 3, 8, 8};
  ASSERT_EQ(m.dot(b), e);
}

TEST_F(TestSparseMatrix, CsrDotProduct) {
  cask::CsrMatrix m{cask::DokMatrix{
      1, 0, 0, 0,
      1, 0, 1, 0,
      0, 1, 1, 0,
      0, 0, 1, 1
  }};
  cask::Vector b{1, 2, 3, 4};
  cask::Vector e{1, 4, 5, 7};
  ASSERT_EQ(m.dot(b), e);
}

TEST_F(TestVector, VectorSubtract) {
  cask::Vector a{1, 6, 9, 4};
  cask::Vector b{3, 4, 5, 7};
  cask::Vector exp{-2, 2, 4, -3};
  ASSERT_EQ(a - b, exp);
}

TEST_F(TestVector, VectorNorm) {
  cask::Vector a{1, 2, 3, 1, 1};
  ASSERT_EQ(a.norm(), 4);
}
