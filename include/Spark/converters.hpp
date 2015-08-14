#ifndef CONVERTERS_HPP_QNX3RWUJ
#define CONVERTERS_HPP_QNX3RWUJ

#include <stdexcept>
#include <memory>

#include <Spark/SparseMatrix.hpp>
#include <Eigen/Sparse>
#include <Spark/SparseMatrix.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace spark {
  namespace converters {

    // convert an Eigen SparseMatrix to a boost::ublas sparse matrix
    std::unique_ptr<spark::sparse::CsrMatrix<double>> eigenToUblas(
        Eigen::SparseMatrix<double> m) {
      int n = m.cols();
      if (m.rows() != m.cols())
        throw std::invalid_argument("Input matrix is not square!");

      std::unique_ptr<spark::sparse::CsrMatrix<>> a(new spark::sparse::CsrMatrix<>(n, n));

      for (int k=0; k<m.outerSize(); ++k)
        for (Eigen::SparseMatrix<double>::InnerIterator it(m,k); it; ++it)
          (*a)(it.row(), it.col()) = it.value();

      return a;
    }

  }
}


#endif /* end of include guard: CONVERTERS_HPP_QNX3RWUJ */
