// #include <iostream>

// #include <Spark/SparseLinearSolvers.hpp>
// #include <test_utils.hpp>

// int main()
// {
//   int status = -1;
//   // spark::sparse_linear_solvers::DfeCgSolver solver{};
//   // status |= test(16, spark::test::IdentityGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
//   // status |= test(100, spark::test::RandomGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
//   // status |= spark::test::runTest("../test-matrices/bfwb62.mtx", solver);
//   //status |= runMatrixVectorTest(
//       //"../test-matrices/OPF_3754.mtx",
//       //"../test-matrices/OPF_3754_b.mtx");
//   //status |= runMatrixVectorTest(
//       //"../test-matrices/OPF_6000.mtx",
//       //"../test-matrices/OPF_6000_b.mtx");
//   return status;
// }

#include <vector>
#include <Spark/IO.hpp>
#include <Spark/Benchmark.hpp>
#include <Spark/LinearSolvers.hpp>
#include <Spark/SpamSparseMatrix.hpp>
#include <Spark/SpamUtils.hpp>

template<typename P>
void runCg(const spam::SymCsrMatrix &a,
           const vector<double> &exp,
           vector<double> &rhs,
           std::string outFile) {
  int iterations = 0;
  bool verbose = false;
  std::vector<double> sol(a.n);
  spam::Timer t;
  spam::pcg<double, P>(a.matrix, &rhs[0], &sol[0], iterations, verbose, &t);

  ofstream logFile{outFile + ".log"};
  spam::benchmark::printSummary(
      t.get("cg:setup").count(),
      iterations,
      t.get("cg:solve").count(),
      // TODO should measure system residual norm (i.e. l2Norm(A * x - b)
      0,
      spam::benchmark::residual(exp, sol),
      0,
      logFile
  );
  spam::writeToFile(outFile, sol);
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  spam::benchmark::parseArgs(argc, argv);
  spam::SymCsrMatrix a = spam::io::mm::readSymMatrix(argv[2]);
  std::vector<double> rhs = spam::io::mm::readVector(std::string(argv[4]));
  std::vector<double> exp = spam::io::mm::readVector(argv[6]);

  std::cout << "Running without preconditioning " << std::endl;
  runCg<spam::IdentityPreconditioner>(a, exp, rhs, "sol.upc.mtx");
  std::cout << "Running with ILU preconditioning " << std::endl;
  runCg<spam::ILUPreconditioner>(a, exp, rhs, "sol.ilu.mtx");

  return 0;
}
