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


void pretty_print(int inSize, int rowSize, 
                  vector<int> indptr, vector<double> a,  
                  vector<double> out, vector<double> outr,
                  vector<double> vRom, 
                  vector<double> expected,
                  int fpL) {
  cout << "cycle\t        in\t       out\t      outr\t       exp" << endl;
  cout << "-----\t        --\t       ---\t      ----\t       ---" << endl;
  int idx = 0;
  for (int i = 0; i < inSize + fpL; i++) {
    printf("%5d", i);
    if (i >= inSize) {
      cout << "\t         -" ;
    } else {
      printf("\t%10.3f", a[i] * vRom[indptr[i]]);
    }

    if ( i >= inSize || (i % rowSize < fpL && i / rowSize >= 1)) {
      printf("\t%10.3f\t%10.3f", out[idx], outr[idx]);
      idx++;
    } else {
      cout << "\t         -\t         -" ; ;
    }

    if ( i >= inSize || (i % rowSize < fpL && i / rowSize >= 1)) {
      printf("\t%10.3f", expected[i - fpL]);
    } else {
      cout << "\t         -" ;
    }

    cout << endl;
  }
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

  int rowSize = inSize / 2; // 2 rows
  int nRows = inSize / rowSize;

  for (int i = 0; i < vRomSize; i++) {
    vRom[i] = i;
  }

  for(int i = 0; i < inSize; ++i) {
    indptr[i] = i % vRomSize;
    a[i] = i;
    int prev = i % rowSize < fpL ? 0 : expected[i - fpL];
    expected[i] =  prev + a[i] * vRom[indptr[i]];
  }

  // --- Running whole SpMV design
#ifndef fpgaNaive_smStandalone
  std::cout << "Running on DFE." << std::endl;
  fpgaNaive(inSize,
            rowSize,            // scalars
            &indptr[0], &a[0],  // ins
            &out[0],            // outs
            &outr[0],
            &vRom[0]);          // roms
  pretty_print(inSize, rowSize,
               indptr, a,
               out, outr,
               vRom,
               expected,
               fpL);

  // compute expected row sum values
  vector<double> s(inSize / rowSize, 0);
  for (int i = 0; i < s.size(); i++) {
    for (int j = i * rowSize; j < (i + 1) * rowSize; j++)
      s[i] += a[j] * vRom[indptr[j]];
  }

  // check output values
  for (int i = 0; i < nRows; i++)
    for (int j = 0; j < fpL; j++) {
      int expIdx = (i + 1) * rowSize + j - fpL;
      int outIdx = i * fpL + j;
      if (abs(out[outIdx] - expected[expIdx]) > 1E-10) {
        printf("Output from DFE did not match CPU: %d : %f != %f\n",
               outIdx, out[outIdx], expected[expIdx]);
        return 1;
      }
    }

  // check reduced values
  for (int i = 0; i < s.size(); i++) {
    int outIdx = i * fpL + fpL - 1;
    if (abs(outr[outIdx] - s[i]) > 1E-10) {
      printf("Reduced sum does not match - expected %f, got  %f",
             s[i], outr[outIdx]);
      return 1;
    }
  }

  std::cout << "Test passed!" << std::endl;
  return 0;
#endif 

  // --- Running control state machine standalone
#ifdef fpgaNaive_smStandalone


  // Load the matrix

  FILE *f = fopen(path, "r");

  int m_n, m_nnzs;
  double* m_values;
  int *m_col_ind, *m_row_ptr;
  read_system_matrix_unsym_csr(f, &m_n, &m_nnzs, &m_col_ind, &m_row_ptr, &m_values);
  fclose(f);

  int ioSize = m_nnzs;
  vector<double> mo_value(ioSize, 0);
  vector<int> mo_rowend(ioSize, 0), mo_indptr(ioSize, 0);

  for (int i = 0; i < m_nnzs; i++) {
    cout << m_col_ind[i] << " ";
  }
  cout << endl;
  for (int i = 0; i < m_nnzs; i++) {
    cout << m_values[i] << " ";
  }
  cout << endl;
  for (int i = 0; i < m_n + 1; i++) {
    cout << m_row_ptr[i] << " ";
  }
  cout << "---" << endl;

  fpgaNaive_SM(m_n,
               m_nnzs,
               m_col_ind, 
               m_row_ptr + 1,
               &mo_indptr[0],
               &mo_rowend[0]);

  for (int i = 0; i < m_nnzs; i++) {
    cout << mo_indptr[i] << " " << mo_rowend[i] << endl;
  }

  // TODO check results are correct
#endif

}
