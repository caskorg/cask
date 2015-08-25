#ifndef SPMV_H
#define SPMV_H

#include <Eigen/Sparse>

namespace spark {
  namespace spmv {
    Eigen::VectorXd dfespmv(
        Eigen::SparseMatrix<double, Eigen::RowMajor> mat,
        Eigen::VectorXd x
        );
  }
}


#endif /* end of include guard: SPMV_H */
