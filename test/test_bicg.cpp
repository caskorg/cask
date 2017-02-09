#include <Spark/SparseLinearSolvers.hpp>
#include <test_utils.hpp>

int main() {

  int status = 0;
  cask::sparse_linear_solvers::DfeBiCgSolver solver{};
  status |= test(16, cask::test::IdentityGenerator{}, cask::test::SimpleVectorGenerator{}, solver);
  status |= test(100, cask::test::RandomGenerator{}, cask::test::SimpleVectorGenerator{}, solver);
  status |= test(10000, cask::test::RandomGenerator{}, cask::test::SimpleVectorGenerator{}, solver);
  status |= cask::test::runTest("../test/matrices/bfwb62.mtx", solver);
  return status;
}
