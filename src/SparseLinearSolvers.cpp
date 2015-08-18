#include <Spark/SparseLinearSolvers.hpp>

extern "C" {
#include <SuiteSparse_config.h>
#include <umfpack.h>
#include "cblas.h"
#include "amd.h"
#include "colamd.h"
#include "ldl.h"
#include "umfpack_get_determinant.h"
}

#include <iostream>
#include <Eigen/Sparse>
#include <Eigen/UmfPackSupport>

Eigen::VectorXd solveBICG(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b
   )
{
  using namespace Eigen;
  BiCGSTAB<SparseMatrix<double>> solver(A);
  return solver.solve(b);
}

Eigen::VectorXd solveLU(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b
   )
{
  using namespace Eigen;
  SparseLU<SparseMatrix<double>> solver;
  solver.analyzePattern(A);
  solver.factorize(A);
  return solver.solve(b);
}

Eigen::VectorXd solveUMFLU(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b

   )
{
  using namespace Eigen;
  UmfPackLU<SparseMatrix<double>> solver;
  solver.analyzePattern(A);
  solver.factorize(A);
  return solver.solve(b);
}

Eigen::VectorXd solveCG(
    const Eigen::SparseMatrix<double>& A,
    const Eigen::VectorXd& b)
{
  Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> cg;
  cg.compute(A);
  return cg.solve(b);
}

void spark::sparse_linear_solvers::EigenSolver::solve(
    const Eigen::SparseMatrix<double>& A,
    double* x, double *b)
{

  int n = A.cols();
  Eigen::VectorXd eb(n), ex(n);
  for (int i = 0; i < n; i++) {
    eb(i) = b[i];
    ex(i) = 0;
  }

  ex = solveCG(A, eb);
  //  ex = solveBICG(A, eb);
  // ex = solveUMFLU(A, eb);
  // ex = solveLU(A, eb);

  for (int i = 0; i < n; i++) {
    x[i] = ex(i);
  }
}
