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

vector<double> SpMV_CPU(char *path, vector<double> v) {
  vector<double> r = SpMV_MKL_sym(path, v);
  vector<double> r2 = SpMV_MKL_unsym(path, v);

  cout << "Checking sym/unsymmetric match...";
  for (int i = 0; i < v.size(); i++) {
    if (!almost_equal(r[i], r2[i])) {
      printf(" r[%d] == %f != r[%d] == %f\n", i, r[i], i, r2[i]);
      exit(1);
    }
  }
  cout << "done!" << endl;
  return r;
}

int main(int argc, char** argv) {

  char* path = check_file(argv);

  int fpL = fpgaNaive_fpL;

  const int inSize = fpL * 4;
  
  int n = 16;
  vector<double> v(n);
  for (int i = 0; i < n; i++)
    v[i] = i;

  vector<double> bExp = SpMV_CPU(path, v);

  std::vector<double> a(inSize), expected(inSize), out(inSize, 0), outr(inSize, 0);

  int vRomSize = 32;
  vector<double> vRom(vRomSize);
  vector<int> indptr(inSize);

  for (int i = 0; i < vRomSize; i++) {
    vRom[i] = i;
  }

  for(int i = 0; i < inSize; ++i) {
    indptr[i] = i % vRomSize;
    a[i] = i;
    int prev = i < fpL ? 0 : expected[i - fpL];
    expected[i] =  prev + a[i] * vRom[indptr[i]];
  }

  std::cout << "Running on DFE." << std::endl;
  fpgaNaive(inSize, &indptr[0], &a[0],  // ins
            &out[0],                          // outs
            &outr[0],
            &vRom[0]);                        // roms


  cout << "cycle\tin\tout\texp\toutr" << endl;
  cout << "-----\t--\t---\t---\t----" << endl;
  for (int i = 0; i < inSize + fpL; i++) {
    if ( i >= inSize) {
      cout << setprecision(4) << i << "\t-" << "\t" << out[i - inSize] << "\t" << expected[i - fpL] << "\t" << outr[i - inSize];
    } else {
      cout << i << "\t" << a[i] * vRom[indptr[i]] << "\t-\t-";
    }
    cout << endl;
  }

  double reducedSumExp = 0;
  for (int i = 0; i < fpL; i++)
    reducedSumExp += expected[inSize - fpL + i];

  for (int i = 0; i < fpL; i++)
    if (abs(out[i] - expected[inSize - fpL + i]) > 1E-10) {
      printf("Output from DFE did not match CPU: %d : %f != %f\n",
             i, out[i], expected[inSize - fpL + i]);
      return 1;
    }

  if (abs(outr[fpL - 1] - reducedSumExp) > 1E-1) {
    printf("Reduced sum does not match - expected %f, got  %f",
           reducedSumExp, outr[fpL-1]);
      return 1;
  }

  std::cout << "Test passed!" << std::endl;
  return 0;
}
