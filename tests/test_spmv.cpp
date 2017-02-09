#include "Spmv.hpp"
#include "SimpleSpmv.hpp"
#include "Io.hpp"
#include "Converters.hpp"
#include "Execution.hpp"
#include "UserInput.hpp"
#include "GeneratedImplSupport.hpp"

#include <string>
#include <boost/filesystem.hpp>

#include <test_utils.hpp>

// relative path to the library dir, this should probably not be hardcoded, but
// it's fine for now, till we find a better solution (e.g. a env. variable
// SPARK_LIB_PATH etc.)
const std::string LIB_DIR = "../lib-generated/";

using namespace std;
using namespace spark::execution;

int test(string path, int implId) {
  std::cout << "File: " << path << std::endl;
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  int cols = eigenMatrix->cols();

  std::cout << "Param MatrixPath " << path << std::endl;

  Eigen::VectorXd x(cols);
  for (int i = 0; i < cols; i++) x[i] = (double)i * 0.25;

  spark::runtime::SpmvImplementationLoader implLoader;
  // TODO find the best architecture somehow
  int maxRows = eigenMatrix->rows();
  spark::runtime::GeneratedSpmvImplementation* deviceImpl;
  if (implId == -1)
    deviceImpl = implLoader.architectureWithParams(maxRows);
  else
    deviceImpl = implLoader.architectureWithId(implId);

  auto a = new spark::spmv::SimpleSpmvArchitecture(deviceImpl);
  // TODO need a consistent way to handle params
  //cout << a->getParams();
  a->preprocess(*eigenMatrix);
  Eigen::VectorXd got = a->dfespmv(x);
  Eigen::VectorXd exp = *eigenMatrix * x;
  delete a;

  auto mismatches =
      spark::test::check(spark::converters::eigenVectorToStdVector(got),
                         spark::converters::eigenVectorToStdVector(exp));

  if (mismatches.empty()) {
    std::cout << "Test passed!" << std::endl;
    return 0;
  }

  spark::test::print_mismatches(mismatches);
  std::cout << "Test failed: " << mismatches.size() << " mismatches "
            << std::endl;
  return 1;
}

int main(int argc, char** argv) {
  cout << "Program arguments:" << endl;
  for (int i = 0; i < argc; i++) cout << "   " << argv[i] << endl;
  // XXX Reasonable argument parsing

  if (argc > 1) {
    int status = -1;
    if (argc == 2) {
      status = test(argv[1], -1);
    } else if (argc == 3) {
      status = test(argv[1], stoi(argv[2]));
    }
    if (status == 0)
      std::cout << "All tests passed!" << std::endl;
    else
      std::cout << "Tests failed!" << std::endl;
    return status;
  }

  return 1;
}
