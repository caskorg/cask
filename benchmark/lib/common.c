#include "common.h"
#include <stdio.h>
#include "mkl_blas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"


void print_array(double *values, int size) {
  int i;
  for (i = 0; i < size; i++) {
    printf("%.10lg\n ", values[i]);
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

/** Reads the dimensions of the matrix (n - number of rows, m - number
    of columns, nnzs - number of non-zeros) from the given Matrix
    Market file.  If the given file contains an array rather than a
    COO matrix, nnzs will be set to n;
*/
void read_mm_matrix_size(FILE *f, int *n, int *m, int *nnzs, MM_typecode* mcode) {
  if (mm_read_banner(f, mcode) != 0) {
    printf("Could not process Matrix Market banner.\n");
    exit(1);
  }

  if (mm_is_array(*mcode)) {
    mm_read_mtx_array_size(f, n, m);
    *nnzs = *n;
  } else {
    mm_read_mtx_crd_size(f, n, m, nnzs);
  }
}

/** Reads a matrix market file for a symmetric real valued sparse
    matrix and returns the matrix in 0-indexed CSR form. */
void read_mm_sym_matrix(FILE* f, MM_typecode mcode,
                        int n, int nnzs,
                        double *values, int* col_ind, int *row_ptr
                        ) {

  if (!(mm_is_real(mcode) && mm_is_matrix(mcode) &&
        mm_is_sparse(mcode) && mm_is_symmetric(mcode)) ) {
    printf("First argument must be a symmetric, real-valued, sparse matrix\n");
    printf("Market Market type: [%s]\n", mm_typecode_to_str(mcode));
    exit(1);
  }

  int* I = (int *) malloc(nnzs * sizeof(int));
  int *J = (int *) malloc(nnzs * sizeof(int));
  double *val = (double *) malloc(nnzs * sizeof(double));

  int i;
  for (i=0; i<nnzs; i++) {
    fscanf(f, "%d %d %lg\n", &I[i], &J[i], &val[i]);
    I[i]--;
    J[i]--;
  }

  MKL_INT job[] = {
    1, // convert COO to CSR
    1, // use 1 based indexing (required by mkl_dcsrsymv)
    0,
    0, // Is this used?
    nnzs,
    0
  };

  MKL_INT info;

  mkl_dcsrcoo(job,
              &n,
              values,
              col_ind,
              row_ptr,
              &nnzs,
              val,
              I,
              J,
              &info);

  if (info != 0) {
    printf("CSR to COO conversion failed: not enough memory!\n");
    exit(1);
  }
}

/** Returns the zero indexed array of values. */
void read_mm_array(FILE *f, MM_typecode code, int nnzs, double *values) {
  if (mm_is_symmetric(code)) {
    printf("Array can't be symmetric!\n");
    exit(1);
  }

  int i, x, y;
  double val;
  if (mm_is_array(code)) {
    // Is this actually stored in an array format?
    for (i = 0; i < nnzs; i++) {
      fscanf(f, "%lg", &val);
      values[i] = val;
    }
  } else {
    // the array was stored in COO format
    for (i = 0; i < nnzs; i++) {
      fscanf(f, "%d %d %lg", &x, &y, &val);
      if (y != 1) {
        printf("An array should only have ONE column!\n");
        exit(1);
      }
      values[x - 1] = val;
    }
  }
}

double* read_rhs(FILE* g, int* n, int *nnzs) {
  int vn, vm, vnnzs;
  MM_typecode vcode;
  read_mm_matrix_size(g, &vn, &vm, &vnnzs, &vcode);

  double* rhs = malloc(sizeof(double) * vnnzs);
  memset(rhs, 0, sizeof(double) * vnnzs);
  read_mm_array(g, vcode, vnnzs, rhs);

  *n = vn;
  *nnzs = vnnzs;

  return rhs;
}

void read_system_matrix_sym_csr(FILE* f, int* n, int *nnzs, int** col_ind, int** row_ptr, double** values) {
  MM_typecode mcode;

  int m;
  read_mm_matrix_size(f, n, &m, nnzs, &mcode);

  if (*n != m) {
    printf("Matrix should be square!\n");
    exit(1);
  }

  int nzs = *nnzs;
  double *acsr = (double *)malloc(sizeof(double) * nzs);
  MKL_INT *ia = (MKL_INT *)malloc(sizeof(MKL_INT) * (*n + 1)); // row_ref
  MKL_INT *ja = (MKL_INT *)malloc(sizeof(MKL_INT) * nzs); // column indices

  read_mm_sym_matrix(f, mcode, *n, nzs, acsr, ja, ia);

  *row_ptr = ia;
  *col_ind = ja;
  *values = acsr;
}

void write_vector_to_file(const char* filename, double* vector, int size)
{
    int i;
    FILE *fout = fopen(filename, "w");
    fprintf(fout, "%%MatrixMarket matrix array real general\n");
    fprintf(fout, "%%-------------------------------------------------------------------------------\n");
    fprintf(fout, "%d 1\n", size);
    for (i = 0; i < size; i++)
      fprintf(fout, "%.12lf\n", vector[i]);
    fclose(fout);
}
