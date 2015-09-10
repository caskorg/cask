#include <string>
#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>

#include <Eigen/Sparse>
#include <Spark/io.hpp>
#include <dfesnippets/Timing.hpp>

#include <chrono>

using namespace spark::spmv;

void dse (std::string basename, SpmvArchitectureSpace* af, Eigen::SparseMatrix<double, Eigen::RowMajor> mat) {
  int it = 0;
  for (SpmvArchitecture* a = af->begin(); a != af->end(); ) {
    auto start = std::chrono::high_resolution_clock::now();
    a->preprocess(mat); // do spmv?
    std::cout << "Matrix: " << basename << " " << a->to_string();
    std::cout << " ResourceUsage: " << a->getResourceUsage().to_string() << std::endl;
    dfesnippets::timing::print_clock_diff("Took: ", start);
    a = af->operator++();
    //std::cout << a.getResourceUsage().to_string() << std::endl;
  }
}

int run (std::string path) {

  std::size_t pos = path.find_last_of("/");
  std::string basename(path.substr(pos, path.size() - pos));
  std::cout << basename << std::endl;

  spark::io::MmReader<double> m(path);
  auto start = std::chrono::high_resolution_clock::now();
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  dfesnippets::timing::print_clock_diff("Reading took: ", start);

  // XXX memory leak
  std::vector<SpmvArchitectureSpace*> factories{
    new SimpleSpmvArchitectureSpace()
  };

  for (auto sas : factories) {
    dse(basename, sas, *eigenMatrix);
  }

}

int main(int argc, char** argv) {
  std::cout << "Program arguments:" << std::endl;
  for (int i = 0; i < argc; i++)
    std::cout << "   " << argv[i] << std::endl;
  // XXX Reasonable argument parsing

  if (argc > 1) {
    auto start = std::chrono::high_resolution_clock::now();
    int status = run(argv[1]);
    dfesnippets::timing::print_clock_diff("Test took: ", start);
    if (status == 0)
      std::cout << "All tests passed!" << std::endl;
    else
      std::cout << "Tests failed!" << std::endl;
    return status;
  }

  return 1;
}
