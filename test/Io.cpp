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
