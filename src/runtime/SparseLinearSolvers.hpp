#ifndef SPARSE_LINEAR_SOLVERS_HPP
#define SPARSE_LINEAR_SOLVERS_HPP

#include <Eigen/Sparse>

namespace cask {
  namespace sparse_linear_solvers {

    class Solver {
      public:
        virtual void analyze(
            const Eigen::SparseMatrix<double> &A) {
        }

        virtual void preprocess(
            const Eigen::SparseMatrix<double> &A) {
        }

        virtual Eigen::VectorXd solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b) = 0;
    };

    class EigenSolver: public Solver {
      public:
        virtual Eigen::VectorXd solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b);
    };

    class DfeCgSolver: public Solver {
      public:
        virtual Eigen::VectorXd solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b);
    };

    class DfeBiCgSolver: public Solver {
      public:
        virtual Eigen::VectorXd solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b);
    };
  }
}

#endif /* end of include guard: SPARSE_LINEAR_SOLVERS_HPP */

