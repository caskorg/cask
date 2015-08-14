#include <Spark/ConjugateGradient.hpp>
#include <iostream>
#include <boost/numeric/ublas/io.hpp>

std::vector<double> spark::cg::DfeCg::solve(
    spark::sparse::CsrMatrix<> a,
    const std::vector<double>& b
    ) {
  std::vector<double> result(b.size(), 0);
  std::cout << "Calling cg solve" << std::endl;
  std::cout << a << std::endl;
  return result;
}
