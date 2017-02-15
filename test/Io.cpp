#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <gtest/gtest.h>

class TestMmIo : public ::testing::Test { };

TEST_F(TestMmIo, ReadGenericMatrix) {
  cask::CsrMatrix a = cask::io::readMatrix("test/matrices/test_dense_4.mtx");
  EXPECT_EQ(a.n, 4);
  EXPECT_EQ(a.m, 4);
  EXPECT_EQ(a.nnzs, 16);
  cask::DokMatrix dk = a.toDok();
  EXPECT_EQ(dk.at(0,0), 0.160600717781);
  EXPECT_EQ(dk.at(0, 1), 0.714180454286);
  EXPECT_EQ(dk.at(0, 2), 0.606664491138);
  EXPECT_EQ(dk.at(0, 3), 0.131826930446);
  EXPECT_EQ(dk.at(1, 0), 0.72239480913);
  EXPECT_EQ(dk.at(1, 1), 0.214033912785);
  EXPECT_EQ(dk.at(1, 2), 0.283473395702);
  EXPECT_EQ(dk.at(1, 3), 0.706744964761);
  EXPECT_EQ(dk.at(2, 0), 0.611159175245);
  EXPECT_EQ(dk.at(2, 1), 0.00982158850327);
  EXPECT_EQ(dk.at(2, 2), 0.856150788543);
  EXPECT_EQ(dk.at(2, 3), 0.116685229285);
  EXPECT_EQ(dk.at(3, 0), 0.165573216994);
  EXPECT_EQ(dk.at(3, 1), 0.694267122731);
  EXPECT_EQ(dk.at(3, 2), 0.971614586317);
  EXPECT_EQ(dk.at(3, 3), 0.997169318601);
}

TEST_F(TestMmIo, ReadHeader) {
  std::string path = "test/systems/tinysym.mtx";
  cask::io::MmInfo info = cask::io::readHeader(path);
  ASSERT_EQ(info.symmetry, "symmetric");
  ASSERT_EQ(info.format, "coordinate");
  ASSERT_EQ(info.type, "matrix");
  ASSERT_EQ(info.dataType, "real");
}


TEST_F(TestMmIo, ReadSymCsr) {
  cask::SymCsrMatrix a = cask::io::readSymMatrix("test/systems/tiny.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.m, 4);
  ASSERT_EQ(a.nnzs, 4);
  cask::DokMatrix exp{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp);

  a = cask::io::readSymMatrix("test/systems/tinysym.mtx");
  ASSERT_EQ(a.n, 4);
  ASSERT_EQ(a.m, 4);
  ASSERT_EQ(a.nnzs, 6);
  cask::DokMatrix exp2{
      1, 0, 0, 1,
      0, 1, 0, 0,
      0, 0, 1, 0,
      1, 0, 0, 2};
  ASSERT_EQ(a.matrix.toDok().explicitSymmetric(), exp2);
}
