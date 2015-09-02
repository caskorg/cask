#include <Spark/SparseLinearSolvers.hpp>
#include <test_utils.hpp>

int main() {

  int status = 0;
  spark::sparse_linear_solvers::DfeBiCgSolver solver{};
  status |= test(16, spark::test::IdentityGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
  status |= test(100, spark::test::RandomGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
  status |= test(10000, spark::test::RandomGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
  status |= spark::test::runTest("../test-matrices/bfwb62.mtx", solver);
  return status;
}
