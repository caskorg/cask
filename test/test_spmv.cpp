#include "Spmv.hpp"
#include "IO.hpp"
#include "Converters.hpp"
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

int test(string path, int implId) {
  std::cout << "File: " << path << std::endl;
  cask::io::MmReader<double> m(path);
  auto eigenMatrix = cask::converters::tripletToEigen(m.mmreadMatrix(path));
  int cols = eigenMatrix->cols();

  std::cout << "Param MatrixPath " << path << std::endl;

  cask::Vector x(cols);
  for (int i = 0; i < cols; i++) x[i] = (double)i * 0.25;

  cask::runtime::SpmvImplementationLoader implLoader;
  // TODO find the best architecture somehow
  int maxRows = eigenMatrix->rows();
  cask::runtime::GeneratedSpmvImplementation* deviceImpl;
  if (implId == -1)
    deviceImpl = implLoader.architectureWithParams(maxRows);
  else
    deviceImpl = implLoader.architectureWithId(implId);

  cask::spmv::BasicSpmv a(deviceImpl);
  a.preprocess(*eigenMatrix);
  // TODO need a consistent way to handle params
  //cout << a->getParams();
  cask::Vector got = a.spmv(x);

  Eigen::VectorXd ex(cols);
  for (int i = 0; i < cols; i++) ex[i] = (double)i * 0.25;
  Eigen::VectorXd exp = *eigenMatrix * ex;

  auto mismatches =
      cask::test::check(got.data, cask::converters::eigenVectorToStdVector(exp));

  if (mismatches.empty()) {
    std::cout << "Test passed!" << std::endl;
    return 0;
  }

  cask::test::print_mismatches(mismatches);
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
