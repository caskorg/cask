#include <cstdio>
#include <vector>
#include <cstring>
#include <ostream>
#include <iterator>
#include <algorithm>

#include "mkl_rci.h"
#include "mkl_spblas.h"

extern "C" {
#include "common.h"
}
#include "IO.hpp"
#include "BenchmarkUtils.hpp"

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  sparsebench::benchmarkutils::parseArgs(argc, argv);

  FILE *f;
  if ((f = fopen(argv[2], "r")) == NULL) {
    printf("Could not open %s", argv[1]);
    return 1;
  }
  int n, nnzs;
  double* a;
  int *col_ind, *row_ptr;
  read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &a);
  fclose(f);

  std::vector<double> rhs = sparsebench::io::readVector(argv[4]);

  int rci_request, itercount, i;
  itercount = 0;

  std::vector<double> x(n);
  std::vector<double> precon(n);

  int ipar[128];

  double dpar[128], tmp[4 * n];

  int k = 0;
  for (i = 0; i < nnzs; i++) {
      if (col_ind[i] == k+1) {
          precon[k++] = 1.0/a[i];
      }
  }

  dcg_init (&n, &x[0], &rhs[0], &rci_request, ipar, dpar, tmp);
  if (rci_request != 0) {
    printf("Failed on init\n");
    return 1;
  }
  ipar[4] = 1000000; // set maximum iterations
  ipar[8] = 1;     // enable residual test
  ipar[9] = 0;     // no user stopping test
  ipar[10] = 1;    // implement preconditioned CG
  dpar[0] = 1e-10;// set relative error
  dcg_check (&n, &x[0], &rhs[0], &rci_request, ipar, dpar, tmp); // check params are correct

  if (rci_request != 0) {
    printf("parameters incorrectly set\n");
    return 1;
  }

  double residual;
  bool converged = false;

rci:
  dcg(&n, &x[0], &rhs[0], &rci_request, ipar, dpar, tmp);

  residual = dpar[5];

  if (rci_request == 0) {
    dcg_get (&n, &x[0], &rhs[0], &rci_request, ipar, dpar, tmp, &itercount);
    converged = true;
    goto cleanup;
  } else if (rci_request == 1) {
    char tr = 'l';
    mkl_dcsrsymv (&tr, &n, a, row_ptr, col_ind, tmp, &tmp[n]);
    goto rci;
  } else if (rci_request == 3) {
    elementwise_xty(n, &precon[0], &tmp[2*n], &tmp[3*n]);
    goto rci;
  } else {
    printf("Error! Solver returned unexpected code %d\n", rci_request);
    goto cleanup;
  }

cleanup:

  if (converged) {
    FILE *fout = fopen("sol.mtx", "w");
    fprintf(fout, "%%MatrixMarket matrix array real general\n");
    fprintf(fout, "%%-------------------------------------------------------------------------------\n");
    fprintf(fout, "%d 1\n", n);
    for (i = 0; i < n; i++)
      fprintf(fout, "%.12lf\n", x[i]);
    fclose(fout);
  } else {
    printf("[FAIL] Hasn't converged %d iterations!\n", itercount);
  }

  mkl_free_buffers();

  std::vector<double> exp = sparsebench::io::readVector(argv[6]);

  sparsebench::benchmarkutils::printSummary(
      0,
      itercount,
      0,
      0,
      sparsebench::benchmarkutils::residual(x, exp),
      0
  );

  return 1;
}
