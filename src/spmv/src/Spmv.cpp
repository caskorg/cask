#include <Spark/Spmv.hpp>
#include <Spark/converters.hpp>
#include <iostream>
#include <tuple>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include <dfesnippets/VectorUtils.hpp>
#include <Maxfiles.h>

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

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

  vector<double> v = spark::converters::eigenVectorToStdVector(x);
  int i = 0;
  std::vector<double> total(v.size(), 0);
  int vidx = 0;
  for (auto p : result) {
    auto p_colptr = std::get<0>(p);
    auto p_indptr = std::get<1>(p);
    auto p_values = std::get<2>(p);
    vector<double> out(n, 0);
    int cycles = cycleCount(&p_colptr[0], n);
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
        paddingCycles,
        totalCycles,
        vector_load_cycles,
        n, //uint64_t inscalar_csrDecoder_nrows,
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

    for (int i = 0; i < n; i++)
      total[i] += out[i];
    vidx += Spmv_cacheSize;
  }

  return spark::converters::stdvectorToEigen(total);
}

int spark::spmv::getPartitionSize() {
  return Spmv_cacheSize;
}
