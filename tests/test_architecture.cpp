#include <string>
#include <Spark/Spmv.hpp>
#include <Eigen/Sparse>
#include <Spark/io.hpp>

using namespace spark::spmv;

void dse (SpmvArchitectureSpace* af, Eigen::SparseMatrix<double, Eigen::RowMajor> mat) {
  std::cout << af->end() << std::endl;
  int it = 0;
  for (SpmvArchitecture* a = af->begin(); a != af->end(); ) {
    std::cout << "To stringing" << std::endl;
    std::cout << a->to_string() << std::endl;
    a->preprocessMatrix(mat); // do spmv?
    std::cout << "Estimated flops: " << a->getEstimatedGFlops() << std::endl;
    std::cout << "Here 1" << std::endl;
    std::cout << a << std::endl;
    std::cout << af << std::endl;
    a = af->operator++();
    //std::cout << a.getResourceUsage().to_string() << std::endl;
  }
  std::cout << "Here 2" << std::endl;
}

int run (std::string path) {

  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));

  // XXX memory leak
  std::vector<SpmvArchitectureSpace*> factories{
    new SimpleSpmvArchitectureSpace()
  };

  for (auto sas : factories) {
    dse(sas, *eigenMatrix);
  }

}

int main(int argc, char** argv) {
  std::cout << "Program arguments:" << std::endl;
  for (int i = 0; i < argc; i++)
    std::cout << "   " << argv[i] << std::endl;
  // XXX Reasonable argument parsing

  if (argc > 1) {
    int status = run(argv[1]);
    if (status == 0)
      std::cout << "All tests passed!" << std::endl;
    else
      std::cout << "Tests failed!" << std::endl;
    return status;
  }

  return 1;
}
