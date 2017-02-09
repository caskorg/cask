#include <Eigen/Sparse>
#include <Spark/SparseLinearSolvers.hpp>
#include <Maxfiles.h>
#include <iostream>

//std::vector<double> dfeImpl(
    //cask::sparse::CsrMatrix<> a,
    //const std::vector<double>& bvec) {
  //std::cout << "Calling DFE cg solve" << std::endl;
  //if (a.size1() != a.size2() || a.size1() < 16)
    //throw std::invalid_argument("Matrix must be square and at least 16 x 16 large");

  //using namespace boost::numeric;

  //ublas::vector<double> x(bvec.size(), 0);

  //ublas::vector<double> b(bvec.size());
  //std::copy(bvec.begin(), bvec.end(), b.begin());

  //int maxIter = 100;
  //int iterations = 0;

  //ublas::vector<double> r = b - prod(a, x);
  //auto p = r;
  //auto rsold = inner_prod(r, r);

  //for (int i = 0; i < maxIter; i++) {
    //ublas::vector<double> ap = prod(a, p);
    //double alpha = rsold / inner_prod(p, ap);

    //std::vector<double> apv(ap.begin(), ap.end());
    //std::vector<double> pv(p.begin(), p.end());
    //std::vector<double> rv(r.begin(), r.end());
    //std::vector<double> xv(x.begin(), x.end());
    //std::vector<double> newrv(x.size());
    //std::vector<double> newxv(x.size());

    //int nticks = b.size();
    //std::vector<double> rsnewv(ConjugateGradient_RSNEW_LOOP_LAT);
    //ConjugateGradient(
        //nticks,
        //alpha,
        //b.size(),
        //&ap[0], ap.size() * sizeof(double),
        //&p[0], p.size() * sizeof(double),
        //&r[0], r.size() * sizeof(double),
        //&x[0], x.size() * sizeof(double),
        //&newrv[0], newrv.size() * sizeof(double),
        //&rsnewv[0], rsnewv.size() * sizeof(double),
        //&newxv[0], newxv.size() * sizeof(double));

    //std::copy(newxv.begin(), newxv.end(), x.begin());
    //std::copy(newrv.begin(), newrv.end(), r.begin());
    //double rsnew = std::accumulate(rsnewv.begin(), rsnewv.end(), 0.0);
    //if (sqrt(rsnew) < 1e-20)
      //break;
    //p = r + rsnew / rsold * p;
    //rsold = rsnew;
    //iterations++;
  //}

  //std::cout << "Iterations: " << iterations << std::endl;
  //std::vector<double> result(x.begin(), x.end());
  //return result;
//}

//std::vector<double> referenceCpuImpl(
    //cask::sparse::CsrMatrix<> a,
    //const std::vector<double>& bvec) {
  //std::cout << "Calling cg solve" << std::endl;
  //using namespace boost::numeric;

  //ublas::vector<double> x(bvec.size(), 0);

  //ublas::vector<double> b(bvec.size());
  //std::copy(bvec.begin(), bvec.end(), b.begin());

  //int maxIter = 100;
  //int iterations = 0;

  //ublas::vector<double> r = b - prod(a, x);
  //auto p = r;
  //auto rsold = inner_prod(r, r);

  //for (int i = 0; i < maxIter; i++) {
    //ublas::vector<double> ap = prod(a, p);
    //double alpha = rsold / inner_prod(p, ap);
    //x = x + alpha * p;
    //r = r - alpha * ap;
    //auto rsnew = inner_prod(r, r);
    //std::cout << rsnew << std::endl;
    //if (sqrt(rsnew) < 1e-10)
      //break;
    //p = r + rsnew / rsold * p;
    //rsold = rsnew;
    //iterations++;
  //}

  //std::cout << "Iterations: " << iterations << std::endl;
  //std::vector<double> result(x.begin(), x.end());
  //return result;
//}


Eigen::VectorXd cask::sparse_linear_solvers::DfeCgSolver::solve(
            const Eigen::SparseMatrix<double>& A,
            const Eigen::VectorXd& b)
{
  //return dfeImpl(A, bvec);
  Eigen::VectorXd x(b.size());
  return x;
}
