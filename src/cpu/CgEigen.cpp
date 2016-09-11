#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <chrono>
extern "C" {
#include <mmio.h>
}

using namespace std::chrono;
using namespace Eigen;

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

EigenSparseMatrix readMatrix(const std::string path) {

  FILE* f = fopen(path.c_str(), "r");

  int m, n, nz;
  int* ai, *aj;
  double *aval;
  MM_typecode matcode;
  mm_read_mtx_crd((char *)path.c_str(), &m, &n, &nz, &ai, &aj, &aval, &matcode);

  std::cout << "N" << n << std::endl;
  std::cout << "m" << m << std::endl;
  std::cout << "nz" << nz << std::endl;

  EigenSparseMatrix A(n, n);
  std::vector<Eigen::Triplet<double>> trips;

  for (size_t i = 0; i < nz; i++) {
    trips.push_back(Eigen::Triplet<double>(ai[i] - 1, aj[i] - 1, aval[i]));
  }
  A.setFromTriplets(trips.begin(), trips.end());
  std::cout << A << std::endl;
  fclose(f);
  return A;
}

VectorXd readVector(std::string path) {
  FILE* f = fopen(path.c_str(), "r");

  int m, n, nz;
  int* ai, *aj;
  double *aval;
  MM_typecode matcode;
  mm_read_mtx_crd((char *)path.c_str(), &m, &n, &nz, &ai, &aj, &aval, &matcode);

  std::cout << "N" << n << std::endl;
  std::cout << "m" << m << std::endl;
  std::cout << "nz" << nz << std::endl;
  VectorXd v(100);

  fclose(f);
  return v;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage " << argv[0] << " <matrix> <rhs>" <<std::endl;
    return -1;
  }
  int n = 1E7;

  duration<double> time_span;
  high_resolution_clock::time_point t1, t2;
  VectorXd x(n);

  SparseMatrix<double> A = readMatrix(argv[1]);
  VectorXd b = readVector(argv[2]);

  // fill A and b
  ConjugateGradient<SparseMatrix<double> > solver;

  t1 = high_resolution_clock::now();
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
