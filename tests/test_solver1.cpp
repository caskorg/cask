#include <vector>
#include <iostream>
#include <iterator>

#include <Spark/SparseLinearSolvers.hpp>

#include <dfesnippets/blas/Blas.hpp>
#include <Eigen/Sparse>

using Td = Eigen::Triplet<double>;
using Md = Eigen::SparseMatrix<double>;

Md one(int m) {
  std::vector<Td> nnzs;
  for (int i = 0; i < m; i++)
    nnzs.push_back(Td(i, i, 1));

  Md a(m, m);
  a.setFromTriplets(nnzs.begin(), nnzs.end());
  return a;
}

// functor to generate a matrix
struct IdentityGenerator {
  Md operator()(int m) {
    return one(m);
  }
};

struct RandomGenerator {
  Md operator()(int m) {
    return one(m) * 2;
  }
};

template<typename MatrixGenerator>
void test(int m, MatrixGenerator mg) {
  Md a = mg(m);
  Eigen::VectorXd x(m);
  for (int i = 0; i < m; i++)
        x[i] = i;
  Eigen::VectorXd b = a * x;

  // solve with well known solver
  spark::sparse_linear_solvers::EigenSolver es;

  // es.solve(a);
  std::cout << a << std::endl;
  std::cout << x << std::endl;
  std::cout << b << std::endl;

  std::vector<double> sol(m);
  std::vector<double> newb(b.data(), b.data() + b.size());
  es.solve(a, &sol[0], &newb[0]);

  std::cout << "Solution: " << std::endl;
  std::copy(sol.begin(), sol.end(), std::ostream_iterator<double>{std::cout, " "});
  std::cout << std::endl;


  // TODO run and test FPGA implementation
}

int main()
{
  test<IdentityGenerator>(10, IdentityGenerator{});
  test<RandomGenerator>(100, RandomGenerator{});
}
