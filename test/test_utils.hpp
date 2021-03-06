#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <vector>
#include <tuple>
#include <iostream>
#include <iomanip>

#include <boost/io/ios_state.hpp>

#include <Eigen/Sparse>

#include "SparseLinearSolvers.hpp"
#include "Converters.hpp"
#include "IO.hpp"

#include <dfesnippets/NumericUtils.hpp>


namespace cask {
  namespace test {

    using Td = Eigen::Triplet<double>;
    using Vd = Eigen::VectorXd;
    using Md = Eigen::SparseMatrix<double>;

    using MismatchT = std::vector<std::tuple<int, double, double>>;

    MismatchT check(
        const std::vector<double>& got,
        const std::vector<double>& exp
        )
    {
      std::vector<std::tuple<int, double, double>> mismatches;
      for (int i = 0; i < got.size(); i++) {
        if (!dfesnippets::numeric_utils::almost_equal(got[i], exp[i], 1E-8, 1E-11))
          mismatches.push_back(std::make_tuple(i, got[i], exp[i]));
      }

      return mismatches;
    }

    MismatchT check(
        const Eigen::VectorXd& got,
        const Eigen::VectorXd& exp)
    {
      return
        check(cask::converters::eigenVectorToStdVector(got),
            cask::converters::eigenVectorToStdVector(exp));
    }

    void print_mismatches(const MismatchT& mismatches) {
      boost::io::ios_all_saver guard(std::cout);
      if (!mismatches.empty()) {
        std::cerr << "Results didn't match" << std::endl;
        for (int i = 0; i < mismatches.size(); i++) {
          std::cerr << std::fixed << std::setprecision(10);
          std::cerr << "At " << std::get<0>(mismatches[i]);
          std::cerr << " got: " << std::get<1>(mismatches[i]);
          std::cerr << " exp: "  << std::get<2>(mismatches[i]) << std::endl;
        }
      }
    }

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
        cask::sparse_linear_solvers::EigenSolver es;
        Vd sol = es.solve(mat, vd);
        std::cout << "Eigen solver complete!" << std::endl;
        return std::make_tuple(vd, sol);
      }
    };

    template<typename MatrixGenerator, typename RhsGenerator>
      int test(int m, MatrixGenerator mg, RhsGenerator rhsg,
          cask::sparse_linear_solvers::Solver &solver) {
        Md a = mg(m);
        auto tpl =  rhsg(a, m);
        Vd b = std::get<0>(tpl);
        Vd exp = std::get<1>(tpl);

        std::cout << "Running DFE Solver" << std::endl;
        Vd sol = solver.solve(a, b);
        std::cout << "DFE solver complete!" << std::endl;

        auto mismatches = cask::test::check(sol, exp);
        if (mismatches.empty())
          return 0;
        cask::test::print_mismatches(mismatches);

        return 1;
      }

    int runTest(
        std::string path,
        cask::sparse_linear_solvers::Solver& solver) {
      cask::io::MmReader<double> m(path);
      auto eigenMatrix = cask::converters::tripletToEigen(m.mmreadMatrix(path));
      return test(eigenMatrix->rows(),
          EigenMatrixGenerator{*eigenMatrix},
          cask::test::SimpleVectorGenerator{},
          solver);
    }

    int runTest(
        std::string path,
        std::string vectorPath,
        cask::sparse_linear_solvers::Solver& solver) {
      cask::io::MmReader<double> m(path);
      auto eigenMatrix = cask::converters::tripletToEigen(m.mmreadMatrix(path));
      cask::io::MmReader<double> mv(vectorPath);
      auto eigenVector = cask::converters::stdvectorToEigen(mv.readVector());
      return test(eigenMatrix->rows(),
          EigenMatrixGenerator{*eigenMatrix},
          EigenVectorGenerator{eigenVector},
          solver);
    }

  }
}


#endif /* end of include guard: TEST_UTILS_H */
