#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <Spark/SparseLinearSolvers.hpp>
#include <Spark/ConjugateGradient.hpp>
#include <Spark/converters.hpp>
#include <Spark/io.hpp>

#include <boost/numeric/ublas/io.hpp>

#include <dfesnippets/blas/Blas.hpp>
#include <dfesnippets/sparse/utils.hpp>

#include <Eigen/Sparse>

using Td = Eigen::Triplet<double>;
using Vd = Eigen::VectorXd;
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

struct EigenMatrixGenerator {
  Md matrix;
  EigenMatrixGenerator(Md _matrix) : matrix(_matrix) {}
  Md operator()(int m) {
    return matrix;
  }
};

struct SimpleRhsGenerator {
  Vd operator()(Md mat, int m) {
    Eigen::VectorXd x(m);
    for (int i = 0; i < m; i++)
      x[i] = i;
    return mat * x;
  }
};


template<typename MatrixGenerator, typename RhsGenerator>
int test(int m, MatrixGenerator mg, RhsGenerator rhsg) {
  Md a = mg(m);
  Eigen::VectorXd b =  rhsg(a, m);

  spark::sparse_linear_solvers::EigenSolver es;
  std::vector<double> sol(m);
  std::vector<double> newb(b.data(), b.data() + b.size());
  es.solve(a, &sol[0], &newb[0]);

  spark::cg::DfeCg cg{};
  auto ublas = spark::converters::eigenToUblas(a);
  std::vector<double> res = cg.solve(*ublas, newb);

  bool check = std::equal(sol.begin(), sol.end(), res.begin(),
      [](double a, double b) {
        return dfesnippets::utils::almost_equal(a, b, 1E-10, 1E-15);
        });

  if (!check) {
    std::cout << "Results didn't match" << std::endl;
    std::cout << "Solution (EIGEN): " << std::endl;
    std::copy(sol.begin(), sol.end(), std::ostream_iterator<double>{std::cout, " "});
    std::cout << std::endl;
    std::cout << "Solution (DFE): " << std::endl;
    std::copy(res.begin(), res.end(), std::ostream_iterator<double>{std::cout, " "});
    std::cout << std::endl;
    return 1;
  }
  return 0;
}

int main()
{
  //test(16, IdentityGenerator{});
  //test(100, RandomGenerator{});

  spark::io::MmReader<double> m;
  auto matrix = m.mmread("../test-matrices/bfwb62.mtx");
  auto eigenMatrix = spark::converters::tripletToEigen(matrix);

  int status = 0;
  status |= test(eigenMatrix->rows(),
      EigenMatrixGenerator{*eigenMatrix},
      SimpleRhsGenerator{});

  //for_each(matrix.begin(), matrix.end(),
      //[] (std::tuple<int, int, double> tpl) {
      //std::cout << std::get<0>(tpl) << " " << std::get<1>(tpl) << " " << std::get<2>(tpl) << std::endl;
      //});

//  std::cout << *eigenMatrix << std::endl;
  return status;
}
