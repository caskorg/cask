#include <Spark/SparseLinearSolvers.hpp>
#include <iostream>
#include <stdexcept>

using Vd = Eigen::VectorXd;

Eigen::VectorXd spark::sparse_linear_solvers::DfeBiCgSolver::solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b)
{

  std::cout << "Running bicgstab!!!" << std::endl;
  Vd x(b.size());
  Vd p(b.size());
  Vd v(b.size());

  double rho, alpha, omega, beta;
  double rho_prev, omega_prev;

  Vd r =  b - A * x;
  Vd rhat = r;
  rho = rho_prev = alpha = omega = omega_prev = 1;
  x.setZero();
  v.setZero();
  p.setZero();

  int maxIter = 2000000;
  double normErr = 1E-15;

  Vd precon(b.size());
  for (int i = 0; i < b.size(); i++) {
    if (A.diagonal()(i) == 0)
      throw std::invalid_argument("Main diagonal contains zero elements");
    precon[i] = 1/A.diagonal()(i);
  }

  for (int i = 0; i < maxIter; i++) {
    std::cout << "Iteration " << i << std::endl;
    rho = rhat.dot(r);
    beta = rho / rho_prev * alpha / omega_prev;
    p = r + beta * (p - omega_prev * v);
    Vd y = precon.cwiseProduct(p);
    v = A * y;
    alpha = rho / rhat.dot(v);
    Vd s = r - alpha * v;
    if (s.norm() < normErr) {
      return x + alpha * p;
    }
    Vd z = precon.cwiseProduct(s);
    Vd t = A * z;
    Vd tmp = precon.cwiseProduct(t);
    omega = tmp.dot(z) / tmp.dot(tmp);
    x = x + alpha * y + omega * z;
    r = s - omega * t;
    omega_prev = omega;
    rho_prev = rho;
  }

  return x;
}
