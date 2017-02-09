#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SpamSparseMatrix.hpp>
#include <SpamUtils.hpp>
#include <MklLayer.hpp>
#include <gtest/gtest.h>
#include "TestUtils.hpp"

class TestMklLayer : public ::testing::Test { };


TEST_F(TestMklLayer, TestUnitLowerTriangularSolveIdentity) {
  spam::CsrMatrix sys{spam::DokMatrix{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
  }};
  std::vector<double> rhs{1, 2, 3, 4};
  std::vector<double> exp{1, 2, 3, 4};
  auto res = spam::mkl::unittrsolve(sys, rhs, true);
  spam::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}

TEST_F(TestMklLayer, TestUnitLowerTriangularSolve) {
  spam::CsrMatrix sys{spam::DokMatrix{
      1, 0, 0, 0,
      1, 1, 0, 0,
      0, 1, 1, 0,
      1, 0, 0, 1
  }};
  std::vector<double> rhs{-2, 2, 3, 4};
  std::vector<double> exp{-2, 4, -1, 6};
  auto res = spam::mkl::unittrsolve(sys, rhs, true);
  spam::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}

TEST_F(TestMklLayer, TestUnitUpperTriangularSolve) {
  spam::CsrMatrix sys{spam::DokMatrix{
      1, 0, 0, 1,
      0, 1, 1, 0,
      0, 0, 1, 1,
      0, 0, 0, 1
  }};
  std::vector<double> rhs{-2, 2, 3, 4};
  std::vector<double> exp{-6, 3, -1, 4};
  auto res = spam::mkl::unittrsolve(sys, rhs, false);
  spam::test::pretty_print(sys, rhs, exp, res);
  ASSERT_EQ(res, exp);
}
