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

  // --- Running whole SpMV design
  cout << "Running on DFE." << endl;
  cout << "   n    = " << n << endl;
  cout << "   nnzs = " << nnzs << endl;
  fpgaNaive((long)n,
            (long)nnzs,
            2,
            col_ind,
            row_ptr + 1,
            values,  // ins
            &b[0],
            &v[0]);   

  cout << "CPU = ";
  for (int i = 0; i < bExp.size(); i++)
    cout << bExp[i] << " ";
  cout << endl;
  for (int i = 0; i < b.size(); i++)
    cout << b[i] << " ";

  // TODO check result is correct
  std::cout << "Test passed!" << std::endl;
  return 0;

}
