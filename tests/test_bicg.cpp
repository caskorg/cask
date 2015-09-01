#include <Spark/BiConjugateGradient.hpp>

int main() {
  spark::bicg::DfeBiCg bicg{};
  bicg.solve();

  spark::sparse_linear_solvers::EigenSolver solver;
  solver.solve(





  // load system matrix from file
  // generate test case
  // run with eigen
  // run with bicg
  // check results
  return 0;
}
