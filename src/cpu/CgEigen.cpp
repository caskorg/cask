#include <Eigen/Sparse>
#include "EigenUtils.hpp"

int main(int argc, char **argv) {
  using SolverType = Eigen::ConjugateGradient<sparsebench::eigenutils::EigenSparseMatrix>;
  sparsebench::eigenutils::runSolver<SolverType>(argc, argv);
  return 0;
}
