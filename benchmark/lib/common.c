#include "common.h"
#include <stdio.h>
#include "string.h"
#include "mkl_blas.h"
#include "mkl_cblas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"


void print_array(const char* message, double *values, int size) {
  int i;
  printf("%s", message);
  for (i = 0; i < size; i++) {
    printf("%.10lg\n ", values[i]);
  }
  printf ("\n");
}

void print_array_int(const char* message, int *values, int size) {
  int i;
  printf("%s", message);
  for (i = 0; i < size; i++) {
    printf("%d\n", values[i]);
  }
  printf ("\n");
}

void print_matrix(const char* message, double *values, int size) {
  int i, j;
  printf("%s", message);
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
    matrix and returns the matrix in 1-indexed CSR form. */
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

  // read Matrix Market matrix in COO format
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
    1, // use 1 based indexing for CSR matrix (required by mkl_dcsrsymv)
    0,
    0, // Is this used?
    nnzs,
    0
  };

  MKL_INT info;

  // convert COO matrix to CSR
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

/** Reads a matrix market file for a symmetric real valued sparse
    matrix and returns the matrix in 1-indexed CSR form, with ALL
    VALUES INCLUDED (i.e. when setting A(i, j), we also set A(j, i)).
*/
void read_mm_unsym_matrix(FILE* f, MM_typecode mcode,
                        int n, int* nnzs,
                        double *values, int* col_ind, int *row_ptr
                        ) {

  if (!(mm_is_real(mcode) && mm_is_matrix(mcode) &&
        mm_is_sparse(mcode) && mm_is_symmetric(mcode)) ) {
    printf("First argument must be a symmetric, real-valued, sparse matrix\n");
    printf("Market Market type: [%s]\n", mm_typecode_to_str(mcode));
    exit(1);
  }
  
  // read Matrix Market matrix in COO format
  int* I = (int *) malloc(2 * (*nnzs) * sizeof(int));
  int *J = (int *) malloc(2 * (*nnzs) * sizeof(int));
  double *val = (double *) malloc(2 * (*nnzs) * sizeof(double));

  int i;
  int oldnnzs = *nnzs;
  for (i=0; i<oldnnzs; i++) {
    fscanf(f, "%d %d %lg\n", &I[i], &J[i], &val[i]);
    I[i]--;
    J[i]--;

    // add symmetric entry
    if (J[i] != I[i]) {
      I[(*nnzs)] = J[i]; 
      J[(*nnzs)] = I[i];
      val[(*nnzs)] = val[i];
      (*nnzs)++;
    }

  }

  MKL_INT job[] = {
    1, // convert COO to CSR
    1, // use 1 based indexing for CSR matrix (required by mkl_dcsrsymv)
    0,
    0, // Is this used?
    (*nnzs),
    0
  };

  MKL_INT info;

  // convert COO matrix to CSR
  mkl_dcsrcoo(job,
              &n,
              values,
              col_ind,
              row_ptr,
              nnzs,
              val,
              I,
              J,
              &info);

  if (info != 0) {
    printf("CSR to COO conversion failed: not enough memory!\n");
    exit(1);
  }
}


/** returns the zero indexed array of values. */
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

void read_system_matrix_unsym_csr(FILE* f, int* n, int *nnzs, int** col_ind, int** row_ptr, double** values) {
  MM_typecode mcode;

  int m;
  read_mm_matrix_size(f, n, &m, nnzs, &mcode);

  if (*n != m) {
    printf("Matrix should be square!\n");
    exit(1);
  }

  double *acsr = (double *)malloc(2 * sizeof(double) * (*nnzs));
  MKL_INT *ia = (MKL_INT *)malloc(sizeof(MKL_INT) * (*n + 1)); // row_ref
  MKL_INT *ja = (MKL_INT *)malloc(2 * sizeof(MKL_INT) * (*nnzs)); // column indices

  read_mm_unsym_matrix(f, mcode, *n, nnzs, acsr, ja, ia);

  *row_ptr = ia;
  *col_ind = ja;
  *values = acsr;
}


/** Reads a generic sparse matrix in Matrix Market format and converts it to CSR */
void read_ge_mm_csr(char* fname,
                    int* n, int *nnzs,
                    int** col_ind, int** row_ptr, double** values) {
  MKL_INT *ia, *ja;
  double *val;
  mm_read_unsymmetric_sparse(fname, n, n, nnzs,
                             &val, &ia, &ja);

  MKL_INT job[] = {
    1, // convert COO to CSR
    1, // use 1 based indexing for CSR matrix (required by mkl_dcsrsymv)
    0,
    0, // Is this used?
    *nnzs,
    0
  };

  MKL_INT info;
  *values = (double *) malloc(*nnzs * sizeof(double));
  *col_ind = (int *) malloc(*nnzs * sizeof(int));
  *row_ptr = (int *) malloc((*n + 1) * sizeof(int));


  // convert COO matrix to CSR
  mkl_dcsrcoo(job,
              n,
              *values,
              *col_ind,
              *row_ptr,
              nnzs,
              val,
              ia,
              ja,
              &info);

  if (info != 0) {
    printf("CSR to COO conversion failed: not enough memory!\n");
    exit(1);
  }
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

void elementwise_xty(const int n, const double *x, const double *y, double *z)
{
  // calling double precision symmetric banded matrix-vector multiplier
  static const int k = 0; // just the diagonal
  static const double alpha = 1.0;
  static const int lda = 1;
  static const int incx = 1;
  static const double beta = 0.0;
  static const int incy = 1;
  static const CBLAS_ORDER order = CblasRowMajor;
  static const CBLAS_UPLO  uplo  = CblasUpper;

  cblas_dsbmv(order, uplo, n, k, alpha, x, lda, y, incx, beta, z, incy);
}
