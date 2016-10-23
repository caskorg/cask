#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <iterator>
#include "IO.hpp"
#include "common.h"
#include "mkl.h"
#include "BenchmarkUtils.hpp"
#include <unordered_map>

#include "lib/timer.hpp"

using namespace std;

class DokMatrix {

 public:
  int n;
  int nnzs;

  // use of std::map important: values are sorted by column index internaly
  std::unordered_map<int, std::map<int, double>> dok;

  DokMatrix(int _n, int _nnzs) : n(_n), nnzs(_nnzs) {}


  DokMatrix explicitSymmetric() {
      DokMatrix m(n, nnzs);
      for (auto &&s : dok) {
          for (auto &&p : s.second) {
              int i = s.first;
              int j = p.first;
              double value = p.second;
              m.dok[i][j] = value;

              // check the transpose entry does not exist
              if (i == j)
                  continue;
              if (dok.find(j) != dok.end()) {
                  if (dok[j].find(i) != dok[j].end()) {
                      if (dok[j][i] != p.second) {
                          throw std::invalid_argument("Matrix is not symmetric");
                      } else {
                          std::cout << "Warning! Matrix already contains transpose entry for "
                                    << s.first << " " << p.first << std::endl;
                      }
                  }
              }

              // add the transpose entry
              m.dok[j][i] = value;
          }
      }
      return m;
  }

  void pretty_print() {
      for (auto &&s : dok) {
          for (auto &&p : s.second) {
              std::cout << s.first << " " << p.first << " " << p.second << std::endl;
          }
      }
  }
};

// TODO verify preconditions:
// with 1 based indexing
// lower triangular
// row entries in increasing column order
class CsrMatrix {
  // TODO move to include/, make API
 public:
  int nnzs;
  int n;
  std::vector<double> values;
  std::vector<int> col_ind;
  std::vector<int> row_ptr;

  CsrMatrix() : n(0), nnzs(0) {}

  CsrMatrix(const DokMatrix& m) {
      nnzs = m.nnzs;
      n = m.n;
      // row_ptr.resize(n + 1);
      int pos = 1;
      for (int i = 1; i < n + 1; i++) {
          row_ptr.push_back(pos);
          for (auto &entries : m.dok.find(i)->second) {
              col_ind.push_back(entries.first);
              values.push_back(entries.second);
              pos++;
          }
      }
      row_ptr.push_back(nnzs + 2);
  }

  CsrMatrix(int _n, int _nnzs, double* _values, int* _col_ind, int* _row_ptr) :
      n(_n),
      nnzs(_nnzs)
  {
      values.assign(_values, _values + _nnzs);
      col_ind.assign(_col_ind, _col_ind + _nnzs);
      row_ptr.assign(_row_ptr, _row_ptr + n + 1);
  }

  // Prints all matrix values
  void pretty_print() {
      for (int i = 0; i < n; i++) {
          int col_ptr = row_ptr[i];
          for (int k = 0; k < n; k++) {
              if (col_ptr < row_ptr[i + 1] && col_ind[col_ptr - 1] == k + 1) {
                  std::cout << values[col_ptr - 1] << " ";
                  col_ptr++;
                  continue;
              }
              std::cout << "0 ";
          }
          std::cout << endl;
      }
  }

  double& get(int i, int j) {
      for (int k = row_ptr[i - 1]; k < row_ptr[i]; k++) {
          if (col_ind[k - 1] == j) {
              return values[k - 1];
          }
      }

      throw std::invalid_argument("No nonzero at row col:"
                                      + std::to_string(i) + " "
                                      + std::to_string(j));
  }

  bool isNnz(int i, int j) {
      try {
          get(i, j);
      } catch (std::invalid_argument) {
          return false;
      }
      return true;
  }

  bool isSymmetric() {
      return true;
  }

  DokMatrix toDok() {
    DokMatrix m(n, nnzs);
      for (int i = 0; i < n; i++) {
          for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
              m.dok[i + 1][col_ind[k - 1]] = values[k - 1];
          }
      }
    return m;
  }

};

class Preconditioner {
 public:
  virtual std::vector<double> apply(std::vector<double> x) = 0;
};

// Equivalent to un-precontitioned CG
class IdentityPreconditioner : public Preconditioner {
 public:
  virtual std::vector<double> apply(std::vector<double> x) override {
      return x;
  }
};

class ILUPreconditioner {
  CsrMatrix pc;
 public:

  // pre - a is a symmetric matrix
  ILUPreconditioner(CsrMatrix &a) {
      if (!a.isSymmetric())
          throw std::invalid_argument("ILUPreconditioner only supports symmetric CSR matrices");
      pc = a;
      for (int i = 2; i <= a.n; i++) {
          for (int k = 1; k <= i - 1; k++) {
              // update pivot - a[i,k] = a[i, k] / a[k, k]
              if (pc.isNnz(i, k) && pc.isNnz(k, k)) {
                  pc.get(i, k) = pc.get(i, k) / pc.get(k, k);
                  double beta = pc.get(i, k);
                  for (int j = k + 1; j <= pc.n; j++) {
                      // update row - a[i, j] -= a[k, j] * a[i, k]
                      if (pc.isNnz(i, j) && pc.isNnz(k, j)) {
                          pc.get(i, j) = pc.get(i, j) - pc.get(k, j) * beta;
                      }
                  }
              }
          }
      }
  }

  virtual std::vector<double> apply(std::vector<double> x) {
      // TODO
      return x;
  }

  void pretty_print() {
      pc.pretty_print();
  }
};

/**
 *  Standard preconditioned CG, (Saad et al)
 *  https://en.wikipedia.org/wiki/Conjugate_gradient_method
 */
template <typename T, typename Precon>
bool pcg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values,
        double* rhs, double* x, int& iterations, bool verbose = false)
{
    // configuration (TODO Should be exposed through params)
    char tr = 'l';
    int maxiters = 2000;
    double tol = 1E-5;
    Precon precon;

    std::vector<double>    r(n);             // residual
    std::vector<double>    b(rhs, rhs + n);  // rhs
    std::vector<double>    p(n);             //
    std::vector<double>    z(n);             //

    //  r = b - A * x
    mkl_dcsrsymv(&tr, &n, matrix_values, row_ptr, col_ind, &x[0], &r[0]);
    cblas_daxpby(n, 1.0, &b[0], 1, -1.0, &r[0], 1);

    // z = M^-1 * r
    z = precon.apply(r);

    p = z;

    // rsold = r * z
    double rsold = cblas_ddot(n, &r[0], 1, &z[0], 1);

    for (int i = 0; i < maxiters; i++) {
        if (verbose) {
            std::cout << " rsold " << rsold << std::endl;
        }
        std::vector<double> Ap(n);
        // Ap = A * p
        mkl_dcsrsymv (&tr, &n, matrix_values, row_ptr, col_ind, &p[0], &Ap[0]);
        // alpha = rsold / (p * Ap)
        double alpha = rsold / cblas_ddot(n, &p[0], 1, &Ap[0], 1);
        // x = x + alpha * p
        cblas_daxpy(n, alpha, &p[0], 1, &x[0], 1);
        // r = r - alpha * Ap
        cblas_daxpby(n, -alpha, &Ap[0], 1, 1.0, &r[0], 1);

        // z = M^-1 * r
        z = precon.apply(r);

        // rsnew = r * z
        double rsnew = cblas_ddot(n, &r[0], 1, &z[0], 1);

        if (rsnew <= tol * tol) {
            // std::cout << "Found solution" << std::endl;
            print_array("x", &x[0], n);
            return true;
        }

        // p = r + (rsnew/rsold) * p
        cblas_daxpby(n, 1, &z[0], 1, rsnew / rsold, &p[0], 1);
        rsold = rsnew;
        iterations = i;
    }

    return false;
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

    sparsebench::benchmarkutils::parseArgs(argc, argv);

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

    std::vector<double> rhs = sparsebench::io::readVector(std::string(argv[4]));

    std::vector<double> sol(n);

    bool verbose = false;
    int iterations = 0;

    CsrMatrix a{n, nnzs, values, col_ind, row_ptr};
    CsrMatrix b{a};
    ILUPreconditioner pc{a};
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
    CsrMatrix explicitA(a.toDok().explicitSymmetric());
    explicitA.pretty_print();
    std::cout << "--- A (CSR from --> DOK - explicit sym) --- " << std::endl;
    ILUPreconditioner explicitPc{explicitA};
    std::cout << "--- Explicit ILU pc matrix" << std::endl;
    explicitPc.pretty_print();
    std::cout << "--- Explicit ILU pc matrix" << std::endl;

    sparsebench::utils::Timer t;
    t.tic("cg:all");
    bool status = pcg<double, IdentityPreconditioner>(n, nnzs, col_ind, row_ptr, values, &rhs[0], &sol[0], iterations, verbose);
    t.toc("cg:all");

    std::vector<double> exp = sparsebench::io::readVector(argv[6]);
    sparsebench::benchmarkutils::printSummary(
        0,
        iterations,
        t.get("cg:all").count(),
        0,
        sparsebench::benchmarkutils::residual(exp, sol),
        0
    );

    write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());

    mkl_free_buffers ();
    return 1;
}
