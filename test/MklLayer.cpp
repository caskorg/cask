#include <vector>
#include <SparseMatrix.hpp>
#include <MklLayer.hpp>
#include <gtest/gtest.h>
#include "TestUtils.hpp"

class TestMklLayer : public ::testing::Test { };


TEST_F(TestMklLayer, TestUnitLowerTriangularSolveIdentity) {
  cask::CsrMatrix sys{cask::DokMatrix{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
  }};
  std::vector<double> rhs{1, 2, 3, 4};
  std::vector<double> exp{1, 2, 3, 4};
  auto res = cask::mkl::unittrsolve(sys, rhs, true);
  cask::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}

TEST_F(TestMklLayer, TestUnitLowerTriangularSolve) {
  cask::CsrMatrix sys{cask::DokMatrix{
      1, 0, 0, 0,
      1, 1, 0, 0,
      0, 1, 1, 0,
      1, 0, 0, 1
  }};
  std::vector<double> rhs{-2, 2, 3, 4};
  std::vector<double> exp{-2, 4, -1, 6};
  auto res = cask::mkl::unittrsolve(sys, rhs, true);
  cask::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}

TEST_F(TestMklLayer, TestUnitUpperTriangularSolve) {
  cask::CsrMatrix sys{cask::DokMatrix{
      1, 0, 0, 1,
      0, 1, 1, 0,
      0, 0, 1, 1,
      0, 0, 0, 1
  }};
  std::vector<double> rhs{-2, 2, 3, 4};
  std::vector<double> exp{-6, 3, -1, 4};
  auto res = cask::mkl::unittrsolve(sys, rhs, false);
  cask::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}
