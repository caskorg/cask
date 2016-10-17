#include <Eigen/Sparse>
#include "EigenUtils.hpp"

int main(int argc, char **argv) {
  using SolverType = Eigen::ConjugateGradient<sparsebench::eigenutils::EigenSparseMatrix>;
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i] << std::endl;
  }
  sparsebench::eigenutils::runSolver<SolverType>(argc, argv);
  return 0;
}
