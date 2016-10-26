#ifndef SPARSEBENCH_IO_HPP
#define SPARSEBENCH_IO_HPP

#include <string>
#include <iostream>
#include <SparseMatrix.hpp>
#include <regex>
#include <fstream>
#include <cassert>
#include <sstream>

extern "C" {
# include <mmio.h>
#include <common.h>
};

namespace spam {
namespace io {
namespace mm {

/** Support for Matrix Market I/O. http://math.nist.gov/MatrixMarket/formats.html
 *
 * Matrix Market format is:
 *
 * %%MatrixMarket <type> <format> <data type> <symmetry>
 * % Any number of comment lines
 * N M L
 * i_1 j_1 v_1
 * . . . . . .
 * i_L j_L v_L
 *
 * where
 * type = matrix
 * format = coordinate | array
 * data type = real | complex | integer | pattern
 * symmetry = symmetric | skew-symmetric | hermitian | general
 *
 * Note:
 *   pattern, complex, skew-symmetric, hermitian are not supported will throw an exception
 */
struct MmInfo {
  std::string type;
  std::string format;
  std::string dataType;
  std::string symmetry;
  MmInfo(std::string _type, std::string _format, std::string _dataType, std::string _symmetry) :
      type(_type),
      format(_format),
      dataType(_dataType),
      symmetry(_symmetry) {}
  bool isMatrix() {
    return type == "matrix";
  }
};

MmInfo readHeader(std::string path) {
  std::ifstream f{path};
  std::string fileInfo;
  std::getline(f, fileInfo);
  std::regex mmHeaderRe("%%MatrixMarket (matrix|array) (coordinate|array) (real|integer) (symmetric|general)");
  std::smatch m;
  if (regex_match(fileInfo, m, mmHeaderRe))
    return MmInfo{m[1], m[2], m[3], m[4]};
  throw std::invalid_argument("Not a valid MatrixMarket file in " + path);
}

std::vector<double> readVector(std::string path) {
  MmInfo info = readHeader(path);

  assert(info.isMatrix());
  assert(info.symmetry == "general");
  assert(info.dataType == "real");

  // skip header
  std::ifstream f{path};
  std::string line;
  while (std::getline(f, line)) {
    if (line[0] != '%')
      break;
  }

  int n, m;
  std::stringstream s;
  s << line;
  s >> n >> m;
  std::vector<double> v(n);

  if (info.format == "coordinate") {
    int l;
    s >> l;
    for (int i = 0; i < l; i++) {
      int a, b;
      double val;
      f >> a >> b >> val;
      v[a] = val;
    }
    return v;
  }

  assert(info.format == "array");

  s >> n >> m;
  for (int i = 0; i < n; i++) {
    double val;
    f >> val;
    v[i] = val;
  }
}
}

CsrMatrix readMatrix(std::string path) {
  int n, nnzs;
  double* values;
  int *col_ind, *row_ptr;
  FILE *f;
  if (!(f = fopen(path.c_str(), "r")))
    throw std::invalid_argument("Could not open path:" + path);
  read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
  fclose(f);
  return spam::CsrMatrix{n, nnzs, values, col_ind, row_ptr};
}

}
}

#endif //SPARSEBENCH_IO_HPP
