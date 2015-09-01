#include <Spark/SparseLinearSolvers.hpp>
#include <iostream>

Eigen::VectorXd spark::sparse_linear_solvers::DfeBiCgSolver::solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b)
{
  Eigen::VectorXd vd(b.size());
  return vd;
}
