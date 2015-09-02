#ifndef SPMV_H
#define SPMV_H

#include <Spark/SparseMatrix.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace spmv {

    Eigen::VectorXd dfespmv(
        Eigen::SparseMatrix<double, Eigen::RowMajor> mat,
        Eigen::VectorXd x);

    Eigen::VectorXd dfespmv(
        const spark::sparse::PartitionedCsrMatrix& result,
        const Eigen::VectorXd& x);

    int getPartitionSize();

    spark::sparse::PartitionedCsrMatrix partition(
        const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat)
    {

      int partitionSize = getPartitionSize();
      const int* indptr = mat.innerIndexPtr();
      const double* values = mat.valuePtr();
      const int* colptr = mat.outerIndexPtr();
      int rows = mat.rows();
      int cols = mat.cols();

      int nPartitions = cols / partitionSize + (cols % partitionSize == 0 ? 0 : 1);
      //std::cout << "Npartitions: " << nPartitions << std::endl;

      std::vector<spark::sparse::CsrMatrix> partitions(nPartitions);
      std::cout << "Partition"  << std::endl;

      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < nPartitions; j++) {
          auto& p = std::get<0>(partitions[j]);
          if (p.size() == 0)
            p.push_back(0);
          else
            p.push_back(p.back());
        }
        //std::cout << "i = " << i << std::endl;
        //std::cout << "colptr" << colptr[i] << std::endl;
        for (int j = colptr[i]; j < colptr[i+1]; j++) {
          auto& p = partitions[indptr[j] / partitionSize];
          int idxInPartition = indptr[j] - (indptr[j] / partitionSize ) * partitionSize;
          std::get<1>(p).push_back(idxInPartition);
          std::get<2>(p).push_back(values[j]);
          std::get<0>(p).back()++;
        }
      }
      std::cout << "Done Partition" << std::endl;
      return partitions;
    }
  }
}


#endif /* end of include guard: SPMV_H */
