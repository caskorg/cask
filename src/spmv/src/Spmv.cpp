#include <Spark/Spmv.hpp>
#include <Spark/converters.hpp>
#include <iostream>
#include <tuple>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include <dfesnippets/VectorUtils.hpp>
#include <Maxfiles.h>

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;
using CsrMatrix = std::tuple<std::vector<int>, std::vector<int>, std::vector<double>>;

// how many cycles does it take to resolve the accesses
int cycleCount(int32_t* v, int size) {
  int cycles = 0;
  int crtPos = 0;
  int bufferWidth = Spmv_inputWidth;
  for (int i = 0; i < size; i++) {
    int toread = v[i] - v[i - 1];
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

std::vector<CsrMatrix> partition(
    const int* colptr,
    const int* indptr,
    const double* values,
    int cols,
    int rows,
    int nnzs,
    int partitionSize) {

  int nPartitions = cols / partitionSize + (cols % partitionSize == 0 ? 0 : 1);
  //std::cout << "Npartitions: " << nPartitions << std::endl;

  std::vector<CsrMatrix> partitions(nPartitions);
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
    Eigen::VectorXd x
    ) {
  using namespace std;

  mat.makeCompressed();
  vector<double> v = spark::converters::eigenVectorToStdVector(x);

  auto result = partition(
      mat.outerIndexPtr(),
      mat.innerIndexPtr(),
      mat.valuePtr(),
      mat.cols(),
      mat.rows(),
      mat.nonZeros(),
      Spmv_cacheSize);

  int i = 0;
  std::vector<double> total(mat.rows(), 0);
  int vidx = 0;
  for (auto p : result) {
    auto p_colptr = std::get<0>(p);
    auto p_indptr = std::get<1>(p);
    auto p_values = std::get<2>(p);
    vector<double> out(mat.cols(), 0);
    int cycles = cycleCount(&p_colptr[0], mat.rows());
    //std::cout << "  Cycles required to compute" << cycles << std::endl;

    //std::cout << "  partition " << i++ << std::endl;
    //std::cout << "  nrows = " << p_colptr.size() << std::endl;
    //dfesnippets::vectorutils::print_vector(p_colptr);
    //dfesnippets::vectorutils::print_vector(p_indptr);
    //dfesnippets::vectorutils::print_vector(p_values);

    // XXX align to numPIpes
    int alignToBytesDouble = std::max((int)sizeof(double) * Spmv_inputWidth, 384);
    int alignToBytesInt = std::max((int)sizeof(int) * Spmv_inputWidth, 384);

    align(p_values, alignToBytesDouble);
    align(p_indptr, alignToBytesInt);
    align(p_colptr, 16);
    align(v, 16); // XXX should partition
    int outOldSize = out.size();
    int paddingCycles = out.size() % 2;
    align(out, 16);

    Spmv_write(
        p_values.size() * sizeof(double),
        0,
        (uint8_t *)&p_values[0]
        );
    Spmv_write(
        p_indptr.size() * sizeof(int),
        p_values.size() * sizeof(double),
        (uint8_t *)&p_indptr[0]);

    int vector_load_cycles = std::min((int)v.size(), Spmv_cacheSize);
    std::cout << "Running on DFE" << std::endl;
    int totalCycles = vector_load_cycles + cycles;
    Spmv(
        totalCycles + paddingCycles, //uint64_t ticks_SpmvKernel,
        mat.cols(), //uint64_t inscalar_SpmvKernel_n,
        paddingCycles,
        totalCycles,
        vector_load_cycles,
        mat.rows(), //uint64_t inscalar_csrDecoder_nrows,
        &p_colptr[0], //const void *instream_colptr,
        p_colptr.size() * sizeof(int), //size_t instream_size_colptr,
        &v[vidx],
        vector_load_cycles * sizeof(double),
        &out[0],//void *outstream_output,
        out.size() * sizeof(double),
        p_values.size() * sizeof(double), // lmlem_address_indptr
        p_indptr.size() * sizeof(int),
        0,
        p_values.size() * sizeof(double) // lmlem_address_indptr
        );//size_t outstream_size_output);
    //std::cout << "Result" << std::endl;
    //dfesnippets::vectorutils::print_vector(out);
    std::cout << "Done on DFE" << std::endl;

    for (int i = 0; i < mat.rows(); i++)
      total[i] += out[i];
    vidx += Spmv_cacheSize;
  }

  return spark::converters::stdvectorToEigen(total);
}

