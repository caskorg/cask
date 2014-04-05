#include <stdio.h>
#include <stdbool.h>

// Intel MKL libraries
#include "mkl_rci.h"
#include "mkl_blas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"
#include <string.h>


void read_array_from_mm_file(char *filename, bool mm, double *values, int size) {
  FILE *fin = fopen(filename, "r");

  if (fin == NULL) {
    printf("Couldn't open file %s\n", filename);
    exit(1);
  }

  char *line = NULL;
  int read, n;

  int l = 0;
  // skip empty lines
  while ((read = getline(&line, &n, fin)) != -1) {
    if (line[0] != '%')
      break;
    l++;
  }

  l = 0;
  // read the actual values
  while (!feof(fin)) {
    int row, col;
    double val;
    if (mm) {
      fscanf(fin, "%d %d %lf", &row, &col, &val);
      *(values + row - 1) = val;
    } else {
      fscanf(fin, "%lf", &val);
      *(values + l) = val;
      l++;
    }
  }

}

void read_matrix_from_mm_file(char *filename, bool mm,
                              double **vals, int **cols, int **row_idxs,
                              int *size, int* nnzs) {
  FILE *fin = fopen(filename, "r");

  if (fin == NULL) {
    printf("Couldn't open file %s\n", filename);
    exit(1);
  }

  char *line = NULL;
  int read, n;

  int l = 0;
  // skip empty lines
  while ((read = getline(&line, &n, fin)) != -1) {
    if (line[0] != '%')
      break;
    l++;
  }

  int nrows;
  sscanf(line, "%d %d %d", &nrows, size, nnzs);
  *vals = (double *)malloc(sizeof(double) * *nnzs);
  *cols = (int *)malloc(sizeof(int) * *nnzs);
  *row_idxs = (int *)malloc(sizeof(int) * (nrows + 1));

  int nvals = 0;
  // read the actual vals
  int prev_row = -1;
  while (!feof(fin)) {
    int row, col;
    double val;
    fscanf(fin, "%d %d %lf", &row, &col, &val);
    row--;
    col--;

    *(*vals + nvals) = val;
    *(*cols + nvals) = col + 1;

    if (prev_row != row) {
      int i;
      for (i = prev_row; i < row - 1; i++) {
        *(*row_idxs + i) = prev_row;
      }
      *(*row_idxs + row) = nvals;
    }
    prev_row = row;
    nvals++;
  }

  *(*row_idxs + nrows) = *nnzs + 1;
}

void print_array(double *values, int size) {
  int i;
  for (i = 0; i < size; i++) {
    printf("%.10lf\n ", values[i]);
  }
  printf ("\n");
}

void print_array_int(int *values, int size) {
  int i;
  for (i = 0; i < size; i++) {
    printf("%d\n", values[i]);
  }
  printf ("\n");
}

void print_matrix(double *values, int size) {
  int i, j;
  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++)
      printf("%lf ", values[i * size + j]);
  }
}


// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

  // read data from matrix market files
  if (argc < 3) {
    printf("Usage: ./cg <coeff_matrix> <rhs>\n");
    return -1;
  }

  bool use_mm = false;
  if (argc == 4) {
    use_mm = true;
  }

  int size = 4098;

  double *rhs = malloc(sizeof(double) * size);
  memset(rhs, 0, sizeof(double) * size);
  read_array_from_mm_file(argv[2], use_mm, rhs, size);

  double *vals = NULL;
  int *cols = NULL, *row_idxs = NULL;
  int nnzs;
  read_matrix_from_mm_file(argv[1], use_mm, &vals, &cols, &row_idxs, &size, &nnzs);
  printf("%d %d\n", size, nnzs);

  /* printf("Done reading\n"); */
  print_array(rhs, size);
  /* print_array(vals, nnzs); */
  /* print_array_int(cols, nnzs); */
  print_array_int(row_idxs, size + 1);

  int rci_request, itercount, expected_itercount = 8, i;
  double *a = vals;

  int length = 128;
  double expected_sol[8] = {1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0};

  double *x = (double *)malloc(sizeof(double) * size);
  memset(x, 0, sizeof(double) * size);

  int ipar[128];

  double dpar[128], tmp[4 * size];



  dcg_init (&size, x, rhs, &rci_request, ipar, dpar, tmp);
  if (rci_request != 0) {
    printf("Failed on init\n");
    return 1;
  }

  ipar[4] = 10000; // set maximum iterations
  ipar[8] = 1;     // enable residual test
  ipar[9] = 0;     // no user stopping test
  dpar[0] = 1.E-5; // set relative error

  printf("HERE\n");
  dcg_check (&size, x, rhs, &rci_request, ipar, dpar, tmp); // check params are correct
  if (rci_request != 0) {
    printf("paramters incorrectly set\n");
    return 1;
  }
  printf("HERE\n");

 rci:dcg (&size, x, rhs, &rci_request, ipar, dpar, tmp); // compute solution
  //  print_array(x, size);
  if (rci_request == 0) {
    dcg_get (&size, x, rhs, &rci_request, ipar, dpar, tmp, &itercount);

    printf ("Solution\n");
    print_array(x, size);

    MKL_FreeBuffers ();

    bool good = true;
    /* for (i = 0; i < size && good; i++) */
    /*   good = fabs(expected_sol[i] - x[i]) < 1.0e-12; */

    if (good) {
    printf("[OK] Converged after %d iterations\n", itercount);
    return 0;
  }
  } else if (rci_request == 1) {
    char tr = 'u';
    mkl_dcsrsymv (&tr, &size, a, row_idxs, cols, tmp, &tmp[size]);
    goto rci;
  }

    //  print_array(x, size);
    MKL_FreeBuffers ();
    printf("[FAIL] Hasn't converged!\n");
    return 1;
  }
