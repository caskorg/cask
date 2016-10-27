#ifndef SPAM_MKLLAYER_HPP
#define SPAM_MKLLAYER_HPP

#include <SparseMatrix.hpp>
#include <mkl.h>

namespace spam {

/**
 * A thin layer over some MKL sparse BLAS functions to enable easier testing and integration in other parts of spam.
 * It is preferable to use these functions over MKL since they are less error-prone and easier to use from C++.
 *
 * There are several issues with MKL functions for sparse BLAS which make them fairly error-prone and inconvenient to use:
 * - both 0 and 1 based indexing is enabled for most functions; however some functions (e.g. mkl_dcsrsymv, trsolve etc.)
 *   only support 1 indexing; this is mentioned in the documentation, but there is no explicit verification at compile
 *   time or runtime; this creates serious issues, especially since in C/C++ arrays use 0 based index
 * - there is no decent mechanism for generic implementations; because MKL is a C library, there is no support for templates
 *   hence we have to rely on names only to guess the type signature of a function; in spam it is very important to enable
 *   instantation of the same algorithm on different types to facillitate numerical exploration
 * - most functions take as input raw pointers and there is not OO representation of the matrix / vector objects;
 *
 * The performance overhead is kept to a minimum so that these functions can replace MKL without substantial
 * performance implications in release builds of spam.
 */
namespace mkl {


// Solve Lx = b where L is a unit lower triangular matrix (all diagonal entries of L are 1)
inline std::vector<double> unittrsolve(const CsrMatrix& m,
                                       const std::vector<double> rhs,
                                       bool lowerTriangular)
{
  auto values = m.values;
  auto row_ptr = m.getRowPtrWithOneBasedIndex();
  auto col_ind = m.getColIndWithOneBasedIndex();

  std::vector<double> res(m.n);

  // TODO may have to verify if matrix is unit triangular
  const char triangular = lowerTriangular ? 'l' : 'u';  // lower triangular
  char transpose = 'N';                                 // solve direct, no transpose
  char unitTriangular = 'N';                            // unit triangular matrix
  mkl_dcsrtrsv(
      &triangular,
      &transpose,
      &unitTriangular,
      &m.n,                   // number of rows
      values.data(),
      row_ptr.data(),
      col_ind.data(),
      &rhs[0],
      &res[0]
  );

  return res;
}

}
}
#endif //SPAM_MKLLAYER_HPP
