#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <Spark/SparseLinearSolvers.hpp>
#include <Spark/converters.hpp>
#include <Spark/io.hpp>

#include <boost/numeric/ublas/io.hpp>

#include <dfesnippets/blas/Blas.hpp>
#include <dfesnippets/NumericUtils.hpp>

#include <Eigen/Sparse>
#include <test_utils.hpp>

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


struct EigenVectorGenerator {
  Vd vd;
  EigenVectorGenerator(Vd _vd) : vd(_vd) {}
  std::tuple<Vd, Vd> operator()(Md mat, int m) {
    std::cout << "Running eigen solver" << std::endl;
    spark::sparse_linear_solvers::EigenSolver es;
    Vd sol = es.solve(mat, vd);
    std::cout << "Eigen solver complete!" << std::endl;
    return std::make_tuple(vd, sol);
  }
};

template<typename MatrixGenerator, typename RhsGenerator>
int test(int m, MatrixGenerator mg, RhsGenerator rhsg,
    spark::sparse_linear_solvers::Solver &solver) {
  Md a = mg(m);
  auto tpl =  rhsg(a, m);
  Vd b = std::get<0>(tpl);
  Vd exp = std::get<1>(tpl);

  std::cout << "Running DFE Solver" << std::endl;
  Vd sol = solver.solve(a, b);
  std::cout << "DFE solver complete!" << std::endl;

  auto mismatches = spark::test::check(sol, exp);
  if (mismatches.empty())
    return 0;
  spark::test::print_mismatches(mismatches);

  return 1;
}

int runTest(
    std::string path,
    spark::sparse_linear_solvers::Solver& solver) {
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  return test(eigenMatrix->rows(),
      EigenMatrixGenerator{*eigenMatrix},
      spark::test::SimpleVectorGenerator{},
      solver);
}

int runTest(
    std::string path,
    std::string vectorPath,
    spark::sparse_linear_solvers::Solver& solver) {
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  spark::io::MmReader<double> mv(vectorPath);
  auto eigenVector = spark::converters::stdvectorToEigen(mv.readVector());
  return test(eigenMatrix->rows(),
      EigenMatrixGenerator{*eigenMatrix},
      EigenVectorGenerator{eigenVector},
      solver);
}

int main()
{
  int status = 0;
  spark::sparse_linear_solvers::DfeCgSolver solver{};
  status |= test(16, IdentityGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
  status |= test(100, RandomGenerator{}, spark::test::SimpleVectorGenerator{}, solver);
  status |= runTest("../test-matrices/bfwb62.mtx", solver);
  //status |= runMatrixVectorTest(
      //"../test-matrices/OPF_3754.mtx",
      //"../test-matrices/OPF_3754_b.mtx");
  //status |= runMatrixVectorTest(
      //"../test-matrices/OPF_6000.mtx",
      //"../test-matrices/OPF_6000_b.mtx");
  return status;
}
