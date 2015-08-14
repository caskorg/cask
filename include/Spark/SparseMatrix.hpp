#ifndef SPARSEMATRIX_HPP_1BWMMLC8
#define SPARSEMATRIX_HPP_1BWMMLC8

#include <boost/numeric/ublas/matrix_sparse.hpp>

namespace spark {
  namespace sparse {
    template<typename T = double> using CsrMatrix =
      boost::numeric::ublas::compressed_matrix<T, boost::numeric::ublas::row_major, 0,
      std::vector<unsigned int>
        >;
  }
}

#endif /* end of include guard: SPARSEMATRIX_HPP_1BWMMLC8 */
