#include <Spark/Spmv.hpp>
#include <Spark/io.hpp>
#include <Spark/converters.hpp>
#include <string>

#include <test_utils.hpp>

using namespace std;

int test(string path) {
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));

  int cols = eigenMatrix->cols();

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

int main() {
  int status = 0;
  status |= test("../test-matrices/test_empty_last_rows_small.mtx");
  status |= test("../test-matrices/test_small.mtx");
  status |= test("../test-matrices/test_two_rows.mtx");
  return status;
}
