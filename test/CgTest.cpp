#include <Benchmark.hpp>
#include <IO.hpp>
#include <SparseLinearSolvers.hpp>
#include <vector>
#include <gtest/gtest.h>

using namespace std;

template<typename P>
void runCg(const cask::SymCsrMatrix &a,
           const vector<double> &exp,
           vector<double> &rhs,
           std::string outFile) {
  int iterations = 0;
  bool verbose = false;
  std::vector<double> sol(a.n);
  cask::utils::Timer t;
  cask::sparse_linear_solvers::pcg<double, P>(a.matrix, &rhs[0], &sol[0], iterations, verbose, &t);
  cask::Vector vrhs(rhs);
  double estimatedL2Norm = (a.dot(sol) - vrhs).norm();

  ofstream logFile{outFile + ".log"};
  cask::benchmark::printSummary(
      t.get("cg:setup").count(),
      iterations,
      t.get("cg:solve").count(),
      estimatedL2Norm,
      cask::benchmark::residual(exp, sol),
      0,
      logFile
  );
  cask::writeToFile(outFile, sol);
}

void run_test(std::string matrixPath, std::string lhsPath, std::string rhsPath) {
  cask::SymCsrMatrix a = cask::io::readSymMatrix(matrixPath);
  std::vector<double> rhs = cask::io::readVector(rhsPath);
  std::vector<double> exp = cask::io::readVector(lhsPath);

  std::cout << "Running without preconditioning " << std::endl;
  runCg<cask::sparse_linear_solvers::IdentityPreconditioner>(a, exp, rhs, "sol.upc.mtx");
  std::cout << "Running with ILU preconditioning " << std::endl;
  runCg<cask::sparse_linear_solvers::ILUPreconditioner>(a, exp, rhs, "sol.ilu.mtx");
}

TEST(CgTest, SolveTiny) {
  run_test("test/systems/tiny.mtx", "test/systems/tiny_sol.mtx", "test/systems/tiny_b.mtx");
}

TEST(CgTest, SolveTinySym) {
  run_test("test/systems/tinysym.mtx", "test/systems/tinysym_sol.mtx", "test/systems/tinysym_b.mtx");
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
// int main (int argc, char** argv) {
//   cask::benchmark::parseArgs(argc, argv);
//   run_test(argv[2], argv[4], argv[6]);
//   return 0;
// }
