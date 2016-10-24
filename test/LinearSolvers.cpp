#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>

extern "C" {
#include <mmio.h>
}
#include <common.h>

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  spam::benchmark::parseArgs(argc, argv);

  // read data from matrix market files
  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  FILE *f;
  if ((f = fopen(argv[2], "r")) == NULL) {
    printf("Could not open %s", argv[2]);
    return 1;
  }
  read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
  fclose(f);

  std::vector<double> rhs = spam::io::readVector(std::string(argv[4]));

  std::vector<double> sol(n);

  bool verbose = false;
  int iterations = 0;

   spam::CsrMatrix a{n, nnzs, values, col_ind, row_ptr};
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
  bool status = spam::pcg<double, spam::IdentityPreconditioner>(n, nnzs, col_ind, row_ptr, values, &rhs[0], &sol[0], iterations, verbose);
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
