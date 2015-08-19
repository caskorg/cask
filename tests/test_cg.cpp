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
#include <dfesnippets/NumericUtils.hpp>

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

struct SimpleVectorGenerator {
  std::tuple<Vd, Vd> operator()(Md mat, int m) {
    Eigen::VectorXd x(m);
    for (int i = 0; i < m; i++)
      x[i] = i * 0.25;
    return std::make_tuple(
        mat * x,
        x);
  }
};

struct EigenVectorGenerator {
  Vd vd;
  EigenVectorGenerator(Vd _vd) : vd(_vd) {}
  std::tuple<Vd, Vd> operator()(Md mat, int m) {
    std::cout << "Running eigen solver" << std::endl;
    spark::sparse_linear_solvers::EigenSolver es;
    std::vector<double> sol(m);
    std::vector<double> newb(vd.data(), vd.data() + vd.size());
    es.solve(mat, &sol[0], &newb[0]);
    std::cout << "Eigen solver complete!" << std::endl;
    Vd esol;
    for (int i = 0; i < sol.size(); i++)
      esol(i) = sol[i];
    return std::make_tuple(vd, esol);
  }
};

template<typename MatrixGenerator, typename RhsGenerator>
int test(int m, MatrixGenerator mg, RhsGenerator rhsg) {
  Md a = mg(m);
  auto tpl =  rhsg(a, m);
  Vd b = std::get<0>(tpl);
  Vd exp = std::get<1>(tpl);

  std::cout << "Running DFE solver" << std::endl;
  spark::cg::DfeCg cg{};
  auto ublas = spark::converters::eigenToUblas(a);
  std::cout << "Conversion finished running solve" << std::endl;

  std::vector<double> newb(b.data(), b.data() + b.size());
  std::vector<double> sol(exp.data(), exp.data() + exp.size());
  std::vector<double> res = cg.solve(*ublas, newb);
  std::cout << "DFE solver complete!" << std::endl;

  std::vector<std::tuple<int, double, double>> mismatches;
  for (int i = 0; i < sol.size(); i++) {
    if (!dfesnippets::numeric_utils::almost_equal(res[i], exp[i], 1E-10, 1E-15))
      mismatches.push_back(std::make_tuple(i, res[i], exp[i]));
  }

  if (mismatches.empty())
    return 0;

  std::cout << "Results didn't match" << std::endl;
  for (int i = 0; i < mismatches.size(); i++) {
    std::cout << i << ": " << "Exp: " << std::get<1>(mismatches[i]);
    std::cout << " got: "  << std::get<2>(mismatches[i]) << std::endl;
  }
  return 1;
}

int runTest(std::string path) {
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  return test(eigenMatrix->rows(),
      EigenMatrixGenerator{*eigenMatrix},
      SimpleVectorGenerator{});
}

int runTest(std::string path, std::string vectorPath) {
  spark::io::MmReader<double> m(path);
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  spark::io::MmReader<double> mv(vectorPath);
  auto eigenVector = spark::converters::stdvectorToEigen(mv.readVector());
  return test(eigenMatrix->rows(),
      EigenMatrixGenerator{*eigenMatrix},
      EigenVectorGenerator{eigenVector});
}

int main()
{
  int status = 0;
  status |= test(16, IdentityGenerator{}, SimpleVectorGenerator{});
  status |= test(100, RandomGenerator{}, SimpleVectorGenerator{});
  status |= runTest("../test-matrices/bfwb62.mtx");
  //status |= runMatrixVectorTest(
      //"../test-matrices/OPF_3754.mtx",
      //"../test-matrices/OPF_3754_b.mtx");
  //status |= runMatrixVectorTest(
      //"../test-matrices/OPF_6000.mtx",
      //"../test-matrices/OPF_6000_b.mtx");
  return status;
}
