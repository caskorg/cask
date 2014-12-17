/***
    This is a simple demo project that you can copy to get started.
    Comments blocks starting with '***' and subsequent non-empty lines
    are automatically added to this project's wiki page.
*/

#include <stdio.h>

#include <vector>
#include <iostream>
#include <cstdio>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"
#include "mkl_spblas.h"

#include <iomanip>
#include <common.h>

using namespace std;


vector<double> SpMV_MKL_ge(char *path,
                           vector<double> v) {
  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;

  read_ge_mm_csr(path, &n, &nnzs, &col_ind, &row_ptr, &values);

  vector<double> r(n);
  char tr = 'N';
  mkl_dcsrgemv(&tr, &n, values, row_ptr, col_ind, &v[0], &r[0]);
  
  return r;
}

vector<double> SpMV_MKL_unsym(char *path, 
                              vector<double> v) {
  FILE *f = fopen(path, "r");

  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  read_system_matrix_unsym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
  printf("n: %d, nnzs: %d\n", n, nnzs);

  vector<double> r(n);
  char tr = 'N';
  mkl_dcsrgemv(&tr, &n, values, row_ptr, col_ind, &v[0], &r[0]);

  fclose(f);
  return r;
}


vector<double> SpMV_MKL_sym(char *path, 
                            vector<double> v) {

  FILE *f = fopen(path, "r");

  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
  printf("n: %d, nnzs: %d\n", n, nnzs);

  vector<double> r(n);
  char tr = 'l';
  mkl_dcsrsymv(&tr, &n, values, row_ptr, col_ind, &v[0], &r[0]);

  fclose(f);
  return r;
}


char* check_file(char **argv) {
  FILE *f;
  char *path = argv[3];
  if ((f = fopen(argv[3], "r")) == NULL) {
    cout << "Error opening input file" << endl;
    exit(1);
  }
  fclose(f);
  return argv[3];
}


bool almost_equal(double a, double b) {
  return abs(a - b) < 1E-10;
}


vector<double> SpMV_CPU(char *path, vector<double> v, bool symmetric) {
  vector<double> r2 = SpMV_MKL_unsym(path, v);
  if (symmetric) {
    vector<double> r = SpMV_MKL_sym(path, v);
    cout << "Checking sym/unsymmetric match...";
    for (int i = 0; i < v.size(); i++) {
      if (!almost_equal(r[i], r2[i])) {
        printf(" r[%d] == %f != r[%d] == %f\n", i, r[i], i, r2[i]);
        exit(1);
      }
    }
    cout << "done!" << endl;
  }
  return r2;
}

/** Returns the smallest number greater than bytes that is multiple of k. */
int align(int bytes, int k) {
  return  k * (bytes / k + (bytes % k == 0 ? 0 : 1));
}

int count_empty_rows(int *row_ptr, int n) {
  int prev = row_ptr[0];
  int empty_rows = 0;
  for (int i = 1; i < n; i++) {
    if (prev == row_ptr[i]) {
      empty_rows++;
    }
    prev = row_ptr[i];
  }
  return empty_rows;
}

int main(int argc, char** argv) {

  char* path = check_file(argv);

  // -- Design Parameters
  int fpL = fpgaNaive_fpL;  // adder latency
  
  // -- Matrix Parameters
  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  read_ge_mm_csr(path, &n, &nnzs, &col_ind, &row_ptr, &values);

  // generate multiplicand
  vector<double> v(n);
  for (int i = 0; i < n; i++)
    v[i] = i;

  // find expected result
  vector<double> bExp = SpMV_MKL_ge(path, v);

  std::vector<double> b(n, 0);

  // adjust from 1 indexed CSR (used by MKL) to 0 indexed CSR
  for (int i = 0; i < nnzs; i++)
    col_ind[i]--;

  int empty_rows = count_empty_rows(row_ptr, n);
  // --- Running whole SpMV design
  cout << "Running on DFE." << endl;
  cout << "            n = " << n << endl;
  cout << "         nnzs = " << nnzs << endl;
  cout << "   empty rows = " << empty_rows << endl;
  cout << " total cycles = " << nnzs + empty_rows << endl;

  // stream size must be multiple of 16 bytes
  // padding bytes are ignored in the actual kernel
  int nnzs_bytes = nnzs * sizeof(int);
  int indptr_size  = align(nnzs_bytes, 16);
  int value_size   = align(2 * nnzs_bytes, 16);
  int row_ptr_size = align(n * sizeof(int), 16);

  fpgaNaive(empty_rows,
            indptr_size,
            nnzs,
            row_ptr_size,
            value_size,
            col_ind,
            row_ptr + 1,
            values,  // ins
            &b[0],
            &v[0]);

  cout << "CPU  = ";
  for (int i = 0; i < bExp.size(); i++)
    cout << bExp[i] << " ";
  cout << endl << "FPGA = ";
  for (int i = 0; i < b.size(); i++)
    cout << b[i] << " ";
  cout << endl;

  for (int i = 0; i < b.size(); i++)
    if (!almost_equal(bExp[i], b[i])) {
      cout << "Expected " << bExp[i] << " got: " << b[i] << endl;
      return 1;
    }
  
  std::cout << "Test passed!" << std::endl;
  return 0;

}
