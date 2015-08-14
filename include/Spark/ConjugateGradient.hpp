#ifndef CG_HPP_29EQSWPB
#define CG_HPP_29EQSWPB

#include <vector>
#include <Spark/SparseMatrix.hpp>

namespace spark {
  namespace cg {
    class DfeCg {
      public:

        std::vector<double> solve(
            spark::sparse::CsrMatrix<> a,
            const std::vector<double>& b);
    };
  }
}

#endif /* end of include guard: CG_HPP_29EQSWPB */
