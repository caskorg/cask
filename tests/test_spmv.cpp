#include <Spark/Spmv.hpp>
#include <Spark/io.hpp>
#include <Spark/converters.hpp>
#include <string>

#include <test_utils.hpp>

using namespace std;

int test(string path) {
  std::cout << "File: " << path << std::endl;
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
  status |= test("../test-matrices/test_long_row.mtx");
  status |= test("../test-matrices/test_large_dense.mtx");
  status |= test("../test-matrices/test_break.mtx");
  status |= test("../test-matrices/test_no_empty_rows.mtx");
  status |= test("../test-matrices/test_non_multiple.mtx");
  status |= test("../test-matrices/test_one_row.mtx");
  status |= test("../test-matrices/test_one_value_one_index_per_row.mtx");
  status |= test("../test-matrices/test_tols90.mtx");
  status |= test("../test-matrices/test_cage6.mtx");
  status |= test("../test-matrices/bfwb62.mtx");
  status |= test("../test-matrices/test_tiny_odd.mtx");
  status |= test("../test-matrices/test_tiny.mtx");
  status |= test("../test-matrices/test_some_empty_rows.mtx");
  //status |= test("../test-matrices/TSOPF_RS_b39_c7.mtx");
  //status |= test("../test-matrices/OPF_3754.mtx");
  //status |= test("../test-matrices/OPF_6000.mtx");

  if (status == 0)
    std::cout << "All tests passed!" << std::endl;
  else
    std::cout << "Tests failed!" << std::endl;
  return status;
}

