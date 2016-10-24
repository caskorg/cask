#include <vector>

#include <IO.hpp>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  spam::benchmark::parseArgs(argc, argv);
  spam::CsrMatrix a = spam::io::readMatrix(argv[2]);
  std::vector<double> rhs = spam::io::readVector(std::string(argv[4]));
  std::vector<double> sol(a.n);

  bool verbose = false;
  int iterations = 0;

  spam::CsrMatrix b{a};
  spam::ILUPreconditioner pc{a};
  std::cout << "--- ILU pc matrix" << std::endl;
  pc.pretty_print();
  std::cout << "--- ILU pc matrix" << std::endl;
  std::cout << "--- A (CSR) --- " << std::endl;
  a.pretty_print();
  std::cout << "--- A (CSR) --- " << std::endl;
  std::cout << "--- A (DOK) --- " << std::endl;
  a.toDok().pretty_print();
  std::cout << "--- A (DOK) --- " << std::endl;
  std::cout << "--- A (DOK - explicit sym) --- " << std::endl;
  a.toDok().explicitSymmetric().pretty_print();
  std::cout << "--- A (DOK - explicit sym) --- " << std::endl;
  std::cout << "--- A (CSR from --> DOK - explicit sym) --- " << std::endl;
  spam::CsrMatrix explicitA(a.toDok().explicitSymmetric());
  explicitA.pretty_print();
  std::cout << "--- A (CSR from --> DOK - explicit sym) --- " << std::endl;
  spam::ILUPreconditioner explicitPc{explicitA};
  std::cout << "--- Explicit ILU pc matrix" << std::endl;
  explicitPc.pretty_print();
  std::cout << "--- Explicit ILU pc matrix" << std::endl;

  spam::Timer t;
  t.tic("cg:all");
  bool status = spam::pcg<double, spam::IdentityPreconditioner>(a, &rhs[0], &sol[0], iterations, verbose);
  t.toc("cg:all");

  std::vector<double> exp = spam::io::readVector(argv[6]);
  spam::benchmark::printSummary(
      0,
      iterations,
      t.get("cg:all").count(),
      0,
      spam::benchmark::residual(exp, sol),
      0
  );

  write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());
  mkl_free_buffers ();
  return 0;
}
