#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <chrono>

using namespace std::chrono;
using namespace Eigen;

int main() {
  int n = 1E7;

  duration<double> time_span;
  high_resolution_clock::time_point t1, t2;
  VectorXd x(n), b(n);
  SparseMatrix<double> A(n,n);
  // fill A and b
  std::cout << "Filling matrix" << std::endl;
  t1 = high_resolution_clock::now();
  BiCGSTAB<SparseMatrix<double> > solver;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < 10; j++)
      A.coeffRef(i, rand() % n ) = rand() % 100;

  std::cout << "Setting A" << std::endl;
  solver.compute(A);

  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  std::cout << "Setup took " << time_span.count() << std::endl;

  t1 = high_resolution_clock::now();
  std::cout << "Starting CG iterations" << std::endl;
  for (int i = 0; i < 10; i++) {
    x = solver.solve(b);
    std::cout << "#iterations:     " << solver.iterations() << std::endl;
    std::cout << "estimated error: " << solver.error()      << std::endl;
  }
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  std::cout << "Solve took " << time_span.count() << std::endl;

  return 0;
}
