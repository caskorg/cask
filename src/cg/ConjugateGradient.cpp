#include <Spark/ConjugateGradient.hpp>
#include <iostream>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>

std::vector<double> spark::cg::DfeCg::solve(
    spark::sparse::CsrMatrix<> a,
    const std::vector<double>& bvec) {
  std::cout << "Calling cg solve" << std::endl;
  using namespace boost::numeric;

  ublas::vector<double> x(bvec.size(), 0);

  ublas::vector<double> b(bvec.size());
  std::copy(bvec.begin(), bvec.end(), b.begin());

  int maxIter = 100;
  int iterations = 0;

  ublas::vector<double> r = b - prod(a, x);
  auto p = r;
  auto rsold = inner_prod(r, r);

  for (int i = 0; i < maxIter; i++) {
    ublas::vector<double> ap = prod(a, p);
    double alpha = rsold / inner_prod(p, ap);
    x = x + alpha * p;
    r = r - alpha * ap;
    auto rsnew = inner_prod(r, r);
    std::cout << rsnew << std::endl;
    if (sqrt(rsnew) < 1e-10)
      break;
    p = r + rsnew / rsold * p;
    rsold = rsnew;
    iterations++;
  }

  std::cout << "Iterations: " << iterations << std::endl;
  std::vector<double> result(x.begin(), x.end());
  return result;
}
