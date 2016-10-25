#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
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
}

