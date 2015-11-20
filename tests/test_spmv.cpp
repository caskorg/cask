#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>
#include <Spark/Io.hpp>
#include <Spark/Converters.hpp>
#include <Spark/Execution.hpp>
#include <Spark/UserInput.hpp>
#include <Spark/GeneratedImplSupport.hpp>

#include <string>
#include <boost/filesystem.hpp>

#include <test_utils.hpp>

// relative path to the library dir, this should probably not be hardcoded, but
// it's fine for now, till we find a better solution (e.g. a env. variable
// SPARK_LIB_PATH etc.)
const std::string LIB_DIR = "../lib-generated/";

using namespace std;
using namespace spark::execution;


int test(string path) {
  std::cout << "File: " << path << std::endl;
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  int cols = eigenMatrix->cols();

  std::cout << "Nonzeros: " << eigenMatrix->nonZeros() << std::endl;

  Eigen::VectorXd x(cols);
  for (int i = 0; i < cols; i++)
    x[i] = (double)i * 0.25;

  spark::runtime::SpmvImplementationLoader implLoader;
  // TODO find the best architecture somehow
  int maxRows = eigenMatrix->rows();
  auto deviceImpl = implLoader.architectureWithParams(maxRows);

  auto a = new spark::spmv::SimpleSpmvArchitecture(deviceImpl);

  a->preprocess(*eigenMatrix);
  Eigen::VectorXd got = a->dfespmv(x);
  Eigen::VectorXd exp = *eigenMatrix * x;
  free(a);

  auto mismatches = spark::test::check(
      spark::converters::eigenVectorToStdVector(got),
      spark::converters::eigenVectorToStdVector(exp));

  if (mismatches.empty()) {
    std::cout << "Test passed!" << std::endl;
    return 0;
  }

  spark::test::print_mismatches(mismatches);
  std::cout << "Test failed: " << mismatches.size() << " mismatches " << std::endl;
  return 1;
}

int main(int argc, char** argv) {
  cout << "Program arguments:" << endl;
  for (int i = 0; i < argc; i++)
    cout << "   " << argv[i] << endl;
  // XXX Reasonable argument parsing

  if (argc > 1) {
    int status = test(argv[1]);
    if (status == 0)
      std::cout << "All tests passed!" << std::endl;
    else
      std::cout << "Tests failed!" << std::endl;
    return status;
  }

  return 1;
}

