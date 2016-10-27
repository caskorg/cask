#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <gtest/gtest.h>

class TestMmIo : public ::testing::Test { };

TEST_F(TestMmIo, ReadHeader) {
  std::string path = "test/systems/tinysym.mtx";
  spam::io::mm::MmInfo info = spam::io::mm::readHeader(path);
  ASSERT_EQ(info.symmetry, "symmetric");
  ASSERT_EQ(info.format, "coordinate");
  ASSERT_EQ(info.type, "matrix");
  ASSERT_EQ(info.dataType, "real");
}


TEST_F(TestMmIo, ReadSymCsr) {
  spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("test/systems/tiny.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.nnzs, 4);
  spam::DokMatrix exp{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp);

  a = spam::io::mm::readSymMatrix("test/systems/tinysym.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.nnzs, 6);
  spam::DokMatrix exp2{
      1, 0, 0, 1,
      0, 1, 0, 0,
      0, 0, 1, 0,
      1, 0, 0, 2};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp2);
}