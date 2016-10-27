#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <iterator>
#include "mkl.h"
#include <unordered_map>
#include <map>
#include <SparseMatrix.hpp>

using namespace std;

namespace spam {


class Preconditioner {
 public:
  virtual std::vector<double> apply(std::vector<double> x) = 0;
};

// Equivalent to un-precontitioned CG
class IdentityPreconditioner: public Preconditioner {
 public:
  virtual std::vector<double> apply(std::vector<double> x) override {
      return x;
  }
};

class ILUPreconditioner {
 public:
  CsrMatrix pc;

  // pre - a is a symmetric matrix
  ILUPreconditioner(const CsrMatrix &a) {
      if (!a.isSymmetric())
          throw std::invalid_argument("ILUPreconditioner only supports symmetric CSR matrices");
      pc = a;
      for (int i = 1; i < a.n; i++) {
          for (int k = 0; k < i; k++) {
              // update pivot - a[i,k] = a[i, k] / a[k, k]
              std::cout << i << k << pc.isNnz(i, k) << " " << pc.isNnz(k, k) << std::endl;
              if (pc.isNnz(i, k) && pc.isNnz(k, k)) {
                  pc.get(i, k) = pc.get(i, k) / pc.get(k, k);
                  double beta = pc.get(i, k);
                  std::cout << " i j beta " << i << " " << k << " " << beta << std::endl;
                  for (int j = k + 1; j < pc.n; j++) {
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
template<typename T, typename Precon>
bool pcg(const CsrMatrix& a, double *rhs, double *x, int &iterations, bool verbose = false) {
    // configuration (TODO Should be exposed through params)
    char tr = 'l';
    int maxiters = 2000;
    double tol = 1E-5;
    Precon precon;

    int n = a.n;
    auto values  = a.values;
    auto row_ptr = a.getRowPtrWithOneBasedIndex();
    auto col_ind = a.getColIndWithOneBasedIndex();
    assert(row_ptr[0] == 1 && "Expecting one based indexing for use with mkl_?csrsymv");

    std::vector<double> r(n);             // residual
    std::vector<double> b(rhs, rhs + n);  // rhs
    std::vector<double> p(n);             //
    std::vector<double> z(n);             //

    //  r = b - A * x
    mkl_dcsrsymv(&tr, &n, values.data(), row_ptr.data(), col_ind.data(), &x[0], &r[0]);
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
        mkl_dcsrsymv(&tr, &n, values.data(), row_ptr.data(), col_ind.data(), &p[0], &Ap[0]);
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
            // print_array("x", &x[0], n);
            return true;
        }

        // p = r + (rsnew/rsold) * p
        cblas_daxpby(n, 1, &z[0], 1, rsnew / rsold, &p[0], 1);
        rsold = rsnew;
        iterations = i;
    }

    mkl_free_buffers ();
    return false;
}
}
