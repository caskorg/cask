#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SpamSparseMatrix.hpp>
#include <SpamUtils.hpp>
#include <gtest/gtest.h>

class TestMmIo : public ::testing::Test { };

TEST_F(TestMmIo, ReadHeader) {
  std::string path = "tests/systems/tinysym.mtx";
  cask::io::MmInfo info = cask::io::readHeader(path);
  ASSERT_EQ(info.symmetry, "symmetric");
  ASSERT_EQ(info.format, "coordinate");
  ASSERT_EQ(info.type, "matrix");
  ASSERT_EQ(info.dataType, "real");
}


TEST_F(TestMmIo, ReadSymCsr) {
  cask::SymCsrMatrix a = cask::io::readSymMatrix("tests/systems/tiny.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.nnzs, 4);
  cask::DokMatrix exp{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp);

  a = cask::io::readSymMatrix("tests/systems/tinysym.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.nnzs, 6);
  cask::DokMatrix exp2{
      1, 0, 0, 1,
      0, 1, 0, 0,
      0, 0, 1, 0,
      1, 0, 0, 2};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp2);
}
