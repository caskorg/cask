#include <vector>
#include <IO.hpp>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>

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
  spam::SymCsrMatrix a = spam::io::readSymMatrix(argv[2]);
  std::vector<double> rhs = spam::io::readVector(std::string(argv[4]));
  std::vector<double> exp = spam::io::readVector(argv[6]);

  std::cout << "Running without preconditioning " << std::endl;
  runCg<spam::IdentityPreconditioner>(a, exp, rhs, "sol.upc.mtx");
  std::cout << "Running with ILU preconditioning " << std::endl;
  runCg<spam::ILUPreconditioner>(a, exp, rhs, "sol.ilu.mtx");

  return 0;
}

