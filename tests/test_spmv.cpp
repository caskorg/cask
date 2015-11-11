#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>
#include <Spark/Io.hpp>
#include <Spark/Converters.hpp>
#include <string>
#include <Spark/UserInput.hpp>

#include <test_utils.hpp>
#include <dlfcn.h>

using namespace std;

int test(string path) {
  std::cout << "File: " << path << std::endl;
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  int cols = eigenMatrix->cols();

  std::cout << "Nonzeros: " << eigenMatrix->nonZeros() << std::endl;

  Eigen::VectorXd x(cols);
  for (int i = 0; i < cols; i++)
    x[i] = (double)i * 0.25;

  auto a = new spark::spmv::SimpleSpmvArchitecture();
  //auto a = new spark::spmv::SkipEmptyRowsArchitecture();

  string libName = a->getLibraryName();
  const char *libPath =  ("../lib-generated/" + libName).c_str();
  void *handle = dlopen(
      libPath,
      RTLD_NOW | RTLD_GLOBAL);
  char *err = dlerror();
  if (err) {
    cout << "There were errors loading the library" << endl;
    std::cout << "Path was " << libPath << std::endl;
    cout << dlerror();
  } else {
    std::cout << "Loading dynamic library succesful!" << std::endl;
  }

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

