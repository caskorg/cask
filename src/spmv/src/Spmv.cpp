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
  for (int i = 1; i <= size; i++) {
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
    Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat,
    Eigen::VectorXd x
    ) {
  using namespace std;

  mat.makeCompressed();
  vector<double> v = spark::converters::eigenVectorToStdVector(x);
  int cycles = cycleCount(mat.outerIndexPtr(), mat.cols() );
  vector<double> out(mat.cols(), 0);
  std::cout << "Cycles required to compute" << cycles << std::endl;

  vector<int> indptr, colptr;
  vector<double> values;

  int* outerIndexPtr = mat.outerIndexPtr() + 1;
  double *valuesptr = mat.valuePtr();
  int* indPointer = mat.innerIndexPtr();
  for (int i = 0; i < mat.cols(); i++) {
    colptr.push_back(outerIndexPtr[i]);
  }
  for (int i = 0; i < mat.nonZeros(); i++) {
    values.push_back(valuesptr[i]);
    indptr.push_back(indPointer[i]);
  }

  // XXX align to numPIpes
  int alignToBytesDouble = std::max((int)sizeof(double) * Spmv_inputWidth, 384);
  int alignToBytesInt = std::max((int)sizeof(int) * Spmv_inputWidth, 384);
  align(values, alignToBytesDouble);
  align(indptr, alignToBytesInt);
  align(colptr, 16);
  align(v, 16);
  align(out, 16);

  Spmv_write(
      values.size() * sizeof(double),
      0,
      (uint8_t *)&values[0]
      );
  Spmv_write(
      indptr.size() * sizeof(int),
      values.size() * sizeof(double),
      (uint8_t *)&indptr[0]);

  Spmv(
      v.size() + cycles, //uint64_t ticks_SpmvKernel,
      mat.cols(), //uint64_t inscalar_SpmvKernel_n,
      mat.rows(), //uint64_t inscalar_csrDecoder_nrows,
      &colptr[0], //const void *instream_colptr,
      colptr.size() * sizeof(int), //size_t instream_size_colptr,
      &v[0],
      v.size() * sizeof(double),
      &out[0],//void *outstream_output,
      out.size() * sizeof(double),
      values.size() * sizeof(double), // lmlem_address_indptr
      indptr.size() * sizeof(int),
      0,
      values.size() * sizeof(double) // lmlem_address_indptr
      );//size_t outstream_size_output);

  return spark::converters::stdvectorToEigen(out);
}

