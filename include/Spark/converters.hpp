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

    using UblasSparseMatrix = std::unique_ptr<spark::sparse::CsrMatrix<double>>;
    using EigenSparseMatrix = std::unique_ptr<Eigen::SparseMatrix<double, Eigen::RowMajor>>;

    // convert an Eigen SparseMatrix to a boost::ublas sparse matrix
    UblasSparseMatrix eigenToUblas(
        Eigen::SparseMatrix<double> m) {
      int n = m.cols();
      if (m.rows() != m.cols())
        throw std::invalid_argument("Input matrix is not square!");

      UblasSparseMatrix a(new spark::sparse::CsrMatrix<>(n, n));

      for (int k=0; k<m.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(m,k); it; ++it)
          (*a)(it.row(), it.col()) = it.value();
      }

      return a;
    }

    // convert a Spark COO matrix to an Eigen Sparse Matrix
    EigenSparseMatrix tripletToEigen(spark::sparse::SparkCooMatrix<double> mat) {
      auto coo = mat.data;
      EigenSparseMatrix m(new Eigen::SparseMatrix<double, Eigen::RowMajor>(mat.n, mat.m));
      std::vector<Eigen::Triplet<double>> trips;
      for (int i = 0; i < coo.size(); i++)
        trips.push_back(
            Eigen::Triplet<double>(
              std::get<0>(coo[i]),
              std::get<1>(coo[i]),
              std::get<2>(coo[i])));
      m->setFromTriplets(trips.begin(), trips.end());
      return m;
    }

    Eigen::VectorXd stdvectorToEigen(std::vector<double> v) {
      Eigen::VectorXd m(v.size());
      for (int i = 0; i < v.size(); i++)
        m[i] = v[i];
      return m;
    }

    std::vector<double> eigenVectorToStdVector(Eigen::VectorXd v) {
      std::vector<double> m(v.size());
      for (int i = 0; i < v.size(); i++)
        m[i] = v[i];
      return m;
    }

  }
}


#endif /* end of include guard: CONVERTERS_HPP_QNX3RWUJ */
