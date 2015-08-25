#include <Spark/Spmv.hpp>
#include <iostream>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <Maxfiles.h>

Eigen::VectorXd spark::spmv::dfespmv(
    Eigen::SparseMatrix<double, Eigen::RowMajor> mat,
    Eigen::VectorXd x
    ) {
  // TODO  DFE Impl
  return mat * x;
}

