#include <stdio.h>
#include <stdbool.h>

// Intel MKL libraries
#include "mkl_rci.h"
#include "mkl_blas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"
#include <string.h>

#include "common.h"
#include "mmio.h"


// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  FILE *f, *g;

  // read data from matrix market files
  if (argc < 3) {
    printf("Usage: ./cg <coeff_matrix> <rhs>\n");
    return -1;
  }

  if ((f = fopen(argv[1], "r")) == NULL) {
    printf("Could not open %s", argv[1]);
    return 1;
  }

  if ((g = fopen(argv[2], "r")) == NULL) {
    printf("Could not open %s", argv[2]);
    return 1;
  }

  printf("Read system matrix: ");
  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
  printf("n: %d, nnzs: %d\n", n, nnzs);

  printf("Read RHS: ");
  int vn, vnnzs;
  double* rhs = read_rhs(g, &vn, &vnnzs);
  //  print_array(rhs, vn);
  printf("n: %d, nnzs: %d\n", vn, vnnzs);

  // -- Start solving the system --
  int rci_request, itercount, i;
  itercount = 0;
  double *a = values;

  int size = n;

  double *x = (double *)malloc(sizeof(double) * size);
  double *precon = (double *)malloc(sizeof(double) * size);
  memset(x, 0, sizeof(double) * size);

  int ipar[128];

  double dpar[128], tmp[4 * size];

  for (i = 0; i < size; i++) {
    x[i] = 0;
  }


  // Initialise diagonal preconditioner; run through the matrix:
  //   if current nonzero entry is on diagonal, invert it
  int k = 0;
  for (i = 0; i < nnzs; i++)
  {
      if (col_ind[i] == k+1)
      {
          precon[k++] = 1.0/values[i];
      }
  }



  dcg_init (&size, x, rhs, &rci_request, ipar, dpar, tmp);
  if (rci_request != 0) {
    printf("Failed on init\n");
    return 1;
  }

  /* printf("Matrix values\n"); */
  /* for (i = 0; i < nnzs; i ++) */
  /*   printf("%f ", values[i]); */
  /* printf("\n"); */
  /* for (i = 0; i < nnzs; i ++) */
  /*   printf("%d ", col_ind[i]); */
  /* printf("\n"); */
  /* for (i = 0; i < n + 1; i ++) */
  /*   printf("%d ", row_ptr[i]); */
  /* printf("\n"); */

  ipar[4] = 1000000; // set maximum iterations
  ipar[8] = 1;     // enable residual test
  ipar[9] = 0;     // no user stopping test
  ipar[10] = 1;    // implement preconditioned CG

  dpar[0] = 1e-10;// set relative error

  dcg_check (&size, x, rhs, &rci_request, ipar, dpar, tmp); // check params are correct
  if (rci_request != 0) {
    printf("parameters incorrectly set\n");
    return 1;
  }

 double residual = 0.0;
 rci:dcg (&size, x, rhs, &rci_request, ipar, dpar, tmp); // compute solution
 
 residual = dpar[5];
 
  //  print_array(x, size);
  if (rci_request == 0) {
    dcg_get (&size, x, rhs, &rci_request, ipar, dpar, tmp, &itercount);

    printf ("Solution\n");
    print_array("x = ", x, size);

    FILE *fout = fopen("sol.mtx", "w");
    fprintf(fout, "%%MatrixMarket matrix array real general\n");
    fprintf(fout, "%%-------------------------------------------------------------------------------\n");
    fprintf(fout, "%d 1\n", size);
    for (i = 0; i < size; i++)
      fprintf(fout, "%.12lf\n", x[i]);
    fclose(fout);

    mkl_free_buffers();

    bool good = true;
    /* for (i = 0; i < size && good; i++) */
    /*   good = fabs(expected_sol[i] - x[i]) < 1.0e-12; */

    // check residual

    if (good) {
      printf("[OK] Converged after %d iterations, sq. norm of residual = %1.12f\n", itercount, residual);
      return 0;
    }
  }
  /*---------------------------------------------------------------------------*/
  /* do matrix multiply */
  /*---------------------------------------------------------------------------*/
   else if (rci_request == 1) {
    char tr = 'l';
    mkl_dcsrsymv (&tr, &size, a, row_ptr, col_ind, tmp, &tmp[size]);
    goto rci;
  /*---------------------------------------------------------------------------*/
  /* If rci_request=3, then compute apply the preconditioner matrix C_inverse  */
  /* on vector tmp[2*n] and put the result in vector tmp[3*n]                  */
  /*---------------------------------------------------------------------------*/
  } else if (rci_request == 3)
    {
      elementwise_xty(size, precon, &tmp[2*n], &tmp[3*n]);

      // solve system of equations with preconditioner matrix (a, ja, ia)
      //mkl_dcsrsv (&matdes[2], &n, &one, matdes, a, ja, ia, &ia[1], &tmp[2 * n], &tmp[3 * n]);
      goto rci;
  } else {
    printf("This example FAILED as the solver has returned the ERROR ");
    printf("code %d\n", rci_request);
  }

  if (x) free(x);
  if (precon) free(precon);

  fclose(f);
  fclose(g);
  //  print_array(x, size);
  mkl_free_buffers();
  printf("[FAIL] Hasn't converged %d iterations!\n", itercount);
  return 1;
}
