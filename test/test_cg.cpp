#include <vector>
#include <IO.hpp>
#include <Benchmark.hpp>
#include <SparseLinearSolvers.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <iostream>

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

  ofstream logFile{outFile + ".log"};
  cask::benchmark::printSummary(
      t.get("cg:setup").count(),
      iterations,
      t.get("cg:solve").count(),
      // TODO should measure system residual norm (i.e. l2Norm(A * x - b)
      0,
      cask::benchmark::residual(exp, sol),
      0,
      logFile
  );
  cask::writeToFile(outFile, sol);
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  cask::benchmark::parseArgs(argc, argv);
  cask::SymCsrMatrix a = cask::io::readSymMatrix(argv[2]);
  std::vector<double> rhs = cask::io::readVector(std::string(argv[4]));
  std::vector<double> exp = cask::io::readVector(argv[6]);

  std::cout << "Running without preconditioning " << std::endl;
  runCg<cask::sparse_linear_solvers::IdentityPreconditioner>(a, exp, rhs, "sol.upc.mtx");
  std::cout << "Running with ILU preconditioning " << std::endl;
  runCg<cask::sparse_linear_solvers::ILUPreconditioner>(a, exp, rhs, "sol.ilu.mtx");

  return 0;
}
