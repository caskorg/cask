#include <Spark/Spmv.hpp>
#include <Spark/converters.hpp>
#include <iostream>
#include <tuple>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include <dfesnippets/VectorUtils.hpp>
#include <Maxfiles.h>

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

// how many cycles does it take to resolve the accesses with FST
int cycleCountFST(int32_t* v, int size) {
  int cycles = 0;
  int bufferWidth = Spmv_inputWidth;
  for (int i = 0; i < size; i++) {
    int toread = v[i] - (i > 0 ? v[i - 1] : 0);
    do {
      toread -= std::min(toread, bufferWidth);
      cycles++;
    } while (toread > 0);
  }
  return cycles;
}

// how many cycles does it take to resolve the accesses
int cycleCount(int32_t* v, int size) {
  int cycles = 0;
  int crtPos = 0;
  int bufferWidth = Spmv_inputWidth;
  for (int i = 0; i < size; i++) {
    int toread = v[i] - (i > 0 ? v[i - 1] : 0);
    do {
      int canread = std::min(bufferWidth - crtPos, toread);
      crtPos += canread;
      crtPos %= bufferWidth;
      cycles++;
      toread -= canread;
    } while (toread > 0);
  }
  return cycles;
}

template<typename T>
void align(std::vector<T>& v, int widthInBytes) {
  int limit = widthInBytes / sizeof(T);
  while ((v.size() * sizeof(T)) % widthInBytes != 0 && limit != 0) {
    v.push_back(0);
    limit--;
  }
}

Eigen::VectorXd spark::spmv::dfespmv(
    EigenSparseMatrix mat,
    Eigen::VectorXd x)
{
  mat.makeCompressed();
  return spark::spmv::dfespmv(spark::spmv::partition(mat), x);
}

Eigen::VectorXd spark::spmv::dfespmv(
    const spark::sparse::PartitionedCsrMatrix& result,
    const Eigen::VectorXd& x)
{
  using namespace std;

  // Assume that the system has been correctly defined
  // and that v.size() === result.rows() === result.total_columns()
  int n = x.size();

  if (n > Spmv_maxRows)
    throw std::invalid_argument("Matrix has too many rows - maximum supported: 30000");

  vector<double> v = spark::converters::eigenVectorToStdVector(x);
  std::vector<double> total(v.size(), 0);
  vector<double> m_values;
  vector<int> m_colptr, m_indptr;

  int cycles = 0;
  for (auto p : result) {
    auto p_colptr = std::get<0>(p);
    auto p_indptr = std::get<1>(p);
    auto p_values = std::get<2>(p);
    cycles += cycleCount(&p_colptr[0], n);
    align(p_indptr, sizeof(int) * Spmv_inputWidth);
    align(p_values, sizeof(double) * Spmv_inputWidth);
    std::copy(p_values.begin(), p_values.end(), back_inserter(m_values));
    std::copy(p_indptr.begin(), p_indptr.end(), back_inserter(m_indptr));
    std::copy(p_colptr.begin(), p_colptr.end(), back_inserter(m_colptr));
  }

  int nPartitions = result.size();
  int outSize = n;
  int paddingCycles = n % 2;
  outSize += paddingCycles;

  vector<double> out(outSize , 0);

  align(m_colptr, 16);
  align(m_values, 384);
  align(m_indptr, 384);
  align(v, sizeof(double) * Spmv_cacheSize);

  Spmv_write(
      m_values.size() * sizeof(double),
      0,
      (uint8_t *)&m_values[0]
      );

  Spmv_write(
      m_indptr.size() * sizeof(int),
      m_values.size() * sizeof(double),
      (uint8_t *)&m_indptr[0]);

  int vector_load_cycles = v.size() / nPartitions;
  std::cout << "Running on DFE" << std::endl;
  int totalCycles = cycles + vector_load_cycles * nPartitions;
  std::cout << "Vector load cycles " << vector_load_cycles << std::endl;
  std::cout << "Padding cycles = " << paddingCycles << std::endl;
  std::cout << "Total cycles = " << totalCycles << std::endl;
  std::cout << "Nrows = " << n << std::endl;
  std::cout << "Compute cycles = " << cycles << std::endl;
  std::cout << "Partitions = " << nPartitions << std::endl;
  std::cout << "Expected out size = " << out.size() << std::endl;
  Spmv(
      nPartitions,
      n,
      paddingCycles,
      totalCycles,
      vector_load_cycles,
      v.size(),
      &m_colptr[0], //const void *instream_colptr,
      m_colptr.size() * sizeof(int), //size_t instream_size_colptr,
      &v[0],
      &out[0],//void *outstream_output,
      m_values.size() * sizeof(double), // lmlem_address_indptr
      m_indptr.size() * sizeof(int),
      0,
      m_values.size() * sizeof(double) // lmlem_address_indptr
      );//size_t outstream_size_output);
  std::cout << "Done on DFE" << std::endl;

  total = out;
  return spark::converters::stdvectorToEigen(total);
}

int spark::spmv::getPartitionSize() {
  return Spmv_cacheSize;
}
