#include <Spark/Spmv.hpp>
#include <Spark/io.hpp>
#include <Spark/converters.hpp>
#include <string>

#include <test_utils.hpp>

using namespace std;

int main()
{

  string path("../test-matrices/test_small.mtx");
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));

  int cols = eigenMatrix->cols();
  int rows = eigenMatrix->rows();
  int nnzs = eigenMatrix->nonZeros();
  std::cout << cols << std::endl;
  std::cout << rows << std::endl;
  std::cout << nnzs << std::endl;

  Eigen::VectorXd x(cols);
  for (int i = 0; i < cols; i++)
    x[i] = i * 0.25;

  auto got = spark::spmv::dfespmv(*eigenMatrix, x);
  auto exp = *eigenMatrix * x;

  auto mismatches = spark::test::check(
      spark::converters::eigenVectorToStdVector(got),
      spark::converters::eigenVectorToStdVector(exp));

  if (mismatches.empty()) {
    std::cout << "Test passed!" << std::endl;
    return 0;
  }

  std::cout << "Test failed" << std::endl;
  return 1;
}
