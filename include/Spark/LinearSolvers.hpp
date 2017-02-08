#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <iterator>
#include "mkl.h"
#include "SpamUtils.hpp"
#include <Spark/MklLayer.hpp>
#include <unordered_map>
#include <map>
#include <Spark/SpamSparseMatrix.hpp>
#include <memory>

using namespace std;

namespace spam {

// Equivalent to un-precontitioned CG
class IdentityPreconditioner {
 public:
  IdentityPreconditioner(const CsrMatrix& a) {
      // nothing to do, but maintain a consistent interface
  }

  virtual std::vector<double> apply(const std::vector<double>& x) {
      return x;
  }
};

class ILUPreconditioner {
 public:
  DokMatrix pc;
  CsrMatrix l, u; // lower and upper factors stored independently

  // cached values, row and col ptrs; the latter two are 1 based indexed, as required by unitrrsolve
  std::vector<double> Lvalues, Uvalues;
  std::vector<int> Lrow_ptr, Lcol_ind, Urow_ptr, Ucol_ind;
  std::vector<double> res;

  // pre - a is a symmetric matrix
  ILUPreconditioner(const CsrMatrix &a) {
      if (!a.isSymmetric())
          throw std::invalid_argument("ILUPreconditioner only supports symmetric CSR matrices");
      pc = a.toDok();
      for (int i = 1; i < a.n; i++) {
          if (pc.dok.count(i) == 0)
              continue;

          for (auto&p : pc.dok[i]) {
            int k = p.first;
            if (k >= i)
                break;
            if (!pc.isNnz(k, k))
              continue;
            pc.dok[i][k] = pc.dok[i][k] / pc.dok[k][k];
            double beta = pc.dok[i][k];

            for (auto&p : pc.dok[i]) {
              int j = p.first;
              if (j < k + 1)
                continue;

              if (pc.isNnz(k, j)) {
                pc.dok[i][j] = pc.dok[i][j] - pc.dok[k][j] * beta;
              }
            }
          }

          // simplified original version of the inner loops
          // for (int k = 0; k < i; k++) {
          //     // update pivot - a[i,k] = a[i, k] / a[k, k]
          //     if (pc.isNnz(i, k) && pc.isNnz(k, k)) {
          //         pc.get(i, k) = pc.get(i, k) / pc.get(k, k);
          //         double beta = pc.get(i, k);
          //         for (int j = k + 1; j < pc.n; j++) {
          //             // update row - a[i, j] -= a[k, j] * a[i, k]
          //             if (pc.isNnz(i, j) && pc.isNnz(k, j)) {
          //                 pc.get(i, j) = pc.get(i, j) - pc.get(k, j) * beta;
          //             }
          //         }
          //     }
          // }
      }

      l = CsrMatrix{pc.getLowerTriangular()};
      Lvalues = l.values;
      Lrow_ptr = l.getRowPtrWithOneBasedIndex();
      Lcol_ind = l.getColIndWithOneBasedIndex();
      u = CsrMatrix{pc.getUpperTriangular()};
      Uvalues = u.values;
      Urow_ptr = u.getRowPtrWithOneBasedIndex();
      Ucol_ind = u.getColIndWithOneBasedIndex();
      res = std::vector<double>(l.n);
  }

  virtual std::vector<double> apply(const std::vector<double>& x) {
      // solve z = M^-1 r <==> Mz = r <==> LUz = r
      // solve: Ly = r
      spam::mkl::unittrsolve(Lvalues.data(), Lrow_ptr.data(), Lcol_ind.data(), x, res.data(), true);
      // then solve Uz = y
      auto y = res;
      spam::mkl::unittrsolve(Uvalues.data(), Urow_ptr.data(), Ucol_ind.data(), y, res.data(), false);
      return res;
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
bool pcg(const CsrMatrix& a, double *rhs, double *x, int &iterations, bool verbose = false, Timer* t = nullptr) {
    // configuration (TODO Should be exposed through params)
    char tr = 'l';
    int maxiters = 2000;
    double tol = 1E-5;
    if (t)
      t->tic("cg:setup");
    Precon precon{a};

    int n = a.n;
    auto values  = a.values;
    auto row_ptr = a.getRowPtrWithOneBasedIndex();
    auto col_ind = a.getColIndWithOneBasedIndex();
    assert(row_ptr[0] == 1 && "Expecting one based indexing for use with mkl_?csrsymv");

    std::vector<double> r(n);             // residual
    std::vector<double> b(rhs, rhs + n);  // rhs
    std::vector<double> p(n);             //
    std::vector<double> z(n);             //
    if (t)
      t->toc("cg:setup");

    if (t)
      t->tic("cg:solve");

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
            std::cout << " rsold " << rsold << "iteration " << iterations << "\n";
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
          if (t)
            t->toc("cg:solve");
          return true;
        }

        // p = r + (rsnew/rsold) * p
        cblas_daxpby(n, 1, &z[0], 1, rsnew / rsold, &p[0], 1);
        rsold = rsnew;
        iterations = i;
    }

    mkl_free_buffers ();

    if (t)
      t->toc("cg:solve");
    return false;
}
}
