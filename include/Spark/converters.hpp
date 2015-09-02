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

    using EigenSparseMatrix = std::unique_ptr<Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>>;

    // convert a Spark COO matrix to an Eigen Sparse Matrix
    EigenSparseMatrix tripletToEigen(spark::sparse::SparkCooMatrix<double> mat) {
      auto coo = mat.data;
      EigenSparseMatrix m(new Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>(mat.n, mat.m));
      std::vector<Eigen::Triplet<double>> trips;
      for (size_t i = 0; i < coo.size(); i++)
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

    std::vector<double> eigenVectorToStdVector(const Eigen::VectorXd& v) {
      std::vector<double> m(v.size());
      for (int i = 0; i < v.size(); i++)
        m[i] = v[i];
      return m;
    }

  }
}


#endif /* end of include guard: CONVERTERS_HPP_QNX3RWUJ */
