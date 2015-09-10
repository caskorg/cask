#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>

#include <Spark/converters.hpp>
#include <iostream>
#include <tuple>

#include <dfesnippets/VectorUtils.hpp>
#include <Maxfiles.h>

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

// how many cycles does it take to resolve the accesses
int cycleCount(int32_t* v, int size, int inputWidth) {
  int cycles = 0;
  int crtPos = 0;
  for (int i = 0; i < size; i++) {
    int toread = v[i] - (i > 0 ? v[i - 1] : 0);
    do {
      int canread = std::min(inputWidth - crtPos, toread);
      crtPos += canread;
      crtPos %= inputWidth;
      cycles++;
      toread -= canread;
    } while (toread > 0);
  }
  return cycles;
}

spark::spmv::PartitionResult spark::spmv::SimpleSpmvArchitecture::partition(
    const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat,
    int partitionSize)
{

  const int* indptr = mat.innerIndexPtr();
  const double* values = mat.valuePtr();
  const int* colptr = mat.outerIndexPtr();
  int rows = mat.rows();
  int cols = mat.cols();
  int n = rows;

  int nPartitions = cols / partitionSize + (cols % partitionSize == 0 ? 0 : 1);
  //std::cout << "Npartitions: " << nPartitions << std::endl;

  std::vector<spark::sparse::CsrMatrix> partitions(nPartitions);

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

  if (n > Spmv_maxRows)
    throw std::invalid_argument(
        "Matrix has too many rows - maximum supported: "
        + std::to_string(Spmv_maxRows));

  std::vector<double> v(n, 0);
  std::vector<double> m_values;
  std::vector<int> m_colptr, m_indptr;

  // now we coalesce partitions
  int cycles = 0;
  for (auto p : partitions) {
    auto p_colptr = std::get<0>(p);
    auto p_indptr = std::get<1>(p);
    auto p_values = std::get<2>(p);
    cycles += cycleCount(&p_colptr[0], n, spark::spmv::getInputWidth());
    spark::spmv::align(p_indptr, sizeof(int) * Spmv_inputWidth);
    spark::spmv::align(p_values, sizeof(double) * Spmv_inputWidth);
    std::copy(p_values.begin(), p_values.end(), back_inserter(m_values));
    std::copy(p_indptr.begin(), p_indptr.end(), back_inserter(m_indptr));
    std::copy(p_colptr.begin(), p_colptr.end(), back_inserter(m_colptr));
  }

  spark::spmv::align(m_colptr, 16);
  spark::spmv::align(m_values, 384);
  spark::spmv::align(m_indptr, 384);
  spark::spmv::align(v, sizeof(double) * Spmv_cacheSize);
  std::cout << "Cycles ==== " << cycles << std::endl;
  std::cout << "v.size() ==== " << v.size() << std::endl;

  gflopsCount = 2.0 * (double)mat.nonZeros() / 1E9;
  std::cout << "Here" << std::endl;

  PartitionResult pr;
  pr.nPartitions = nPartitions;
  pr.n = mat.cols();
  pr.paddingCycles = n % 2;
  pr.totalCycles = cycles + v.size();
  pr.vector_load_cycles = v.size() / nPartitions; // per partition
  pr.m_colptr = m_colptr;
  pr.m_values = m_values;
  pr.m_indptr = m_indptr;

  return pr;
}


Eigen::VectorXd spark::spmv::SimpleSpmvArchitecture::dfespmv(Eigen::VectorXd x)
{
  using namespace std;

   //Assume that the system has been correctly defined
   //and that v.size() === result.rows() === result.total_columns()
  int n = x.size();
  vector<double> v = spark::converters::eigenVectorToStdVector(x);
  spark::spmv::align(v, sizeof(double) * Spmv_cacheSize);

  vector<double> out(n + pr.paddingCycles , 0);

  //spark::spmv::align(v, sizeof(double) * Spmv_cacheSize);

  Spmv_write(
      pr.m_values.size() * sizeof(double),
      0,
      (uint8_t *)&pr.m_values[0]
      );

  Spmv_write(
      pr.m_indptr.size() * sizeof(int),
      pr.m_values.size() * sizeof(double),
      (uint8_t *)&pr.m_indptr[0]);

  std::cout << pr.to_string() << std::endl;
  std::cout << "Running on DFE" << std::endl;
  Spmv(
      pr.nPartitions,
      pr.n,
      pr.paddingCycles,
      pr.totalCycles,
      pr.vector_load_cycles,
      v.size(),
      &pr.m_colptr[0], //const void *instream_colptr,
      pr.m_colptr.size() * sizeof(int), //size_t instream_size_colptr,
      &v[0],
      &out[0],//void *outstream_output,
      pr.m_values.size() * sizeof(double), // lmlem_address_indptr
      pr.m_indptr.size() * sizeof(int),
      0,
      pr.m_values.size() * sizeof(double) // lmlem_address_indptr
      );//size_t outstream_size_output);
  std::cout << "Done on DFE" << std::endl;

  return spark::converters::stdvectorToEigen(out);
}

int spark::spmv::getPartitionSize() {
  return Spmv_cacheSize;
}

int spark::spmv::getInputWidth() {
  return Spmv_inputWidth;
}

void spark::spmv::SimpleSpmvArchitecture::preprocess(
    const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) {
  this->mat = mat;
  this->pr = partition(mat, this->cacheSize);
}
