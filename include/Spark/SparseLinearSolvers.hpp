#ifndef SPARSE_LINEAR_SOLVERS_HPP
#define SPARSE_LINEAR_SOLVERS_HPP

#include "Eigen/Sparse"

namespace SparseLinearSolvers {
  class EigenSolver {
    public:
   void solve(
    const Eigen::SparseMatrix<double>& A,
    double* x,
    double *b);
  };
}

#endif /* end of include guard: SPARSE_LINEAR_SOLVERS_HPP */

