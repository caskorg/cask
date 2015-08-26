#include <Spark/Spmv.hpp>
#include <Spark/converters.hpp>
#include <iostream>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <Maxfiles.h>

// how many cycles does it take to resolve the accesses
int cycleCount(int32_t* v, int size) {
  int cycles = 0;
  int crtPos = 0;
  int bufferWidth = Spmv_inputWidth;
  for (int i = 0; i <= size; i++) {
    int toread = v[i] - v[i - 1];
    while (toread > 0) {
      int canread = std::min(bufferWidth - crtPos, toread);
      crtPos += canread;
      crtPos %= bufferWidth;
      cycles++;
      toread -= canread;
    }
  }
  return cycles;
}

Eigen::VectorXd spark::spmv::dfespmv(
    Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat,
    Eigen::VectorXd x
    ) {
  using namespace std;

  mat.makeCompressed();
  vector<double> v = spark::converters::eigenVectorToStdVector(x);
  int cycles = cycleCount(mat.outerIndexPtr(), mat.cols() );
  vector<double> out(16, 0);
  std::cout << "Cycles required to compute" << cycles << std::endl;

  Spmv(
      v.size() + cycles, //uint64_t ticks_SpmvKernel,
      mat.cols(), //uint64_t inscalar_SpmvKernel_n,
      mat.rows(), //uint64_t inscalar_csrDecoder_nrows,
      mat.outerIndexPtr() + 1, //const void *instream_colptr,
      mat.cols() * sizeof(int), //size_t instream_size_colptr,
      mat.innerIndexPtr(), //const void *instream_indptr,
      mat.nonZeros() * sizeof(int), //size_t instream_size_indptr,
      mat.valuePtr(), //const void *instream_values,
      mat.nonZeros() * sizeof(double),//size_t instream_size_values,
      &v[0],
      v.size() * sizeof(double),
      &out[0],//void *outstream_output,
      out.size() * sizeof(double));//size_t outstream_size_output);

  return spark::converters::stdvectorToEigen(out);
}

