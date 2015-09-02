#ifndef SPARSEMATRIX_HPP_1BWMMLC8
#define SPARSEMATRIX_HPP_1BWMMLC8

#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace sparse {
    template<typename T = double> using CsrMatrix =
      boost::numeric::ublas::compressed_matrix<T, boost::numeric::ublas::row_major, 0,
      std::vector<unsigned int>
        >;

    template<typename value_type>
      class SparkCooMatrix {

        public:
        using CoordType = std::tuple<int, int, value_type>;

        int n, m;
        std::vector<CoordType> data;

        SparkCooMatrix(int _n, int _m) : n(_n), m(_m) {}
      };

    Eigen::VectorXd Spmv_dfe(
        const Eigen::SparseMatrix<double>& A,
        const Eigen::VectorXd& b
        ) {
      return A * b;
    }
  }
}

#endif /* end of include guard: SPARSEMATRIX_HPP_1BWMMLC8 */
