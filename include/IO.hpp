#ifndef SPARSEBENCH_IO_HPP
#define SPARSEBENCH_IO_HPP

#include <string>
#include <iostream>
#include <SparseMatrix.hpp>
#include <regex>
#include <fstream>

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
   * type = matrix | array
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
        symmetry(_symmetry)
        { }
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
}

std::vector<double> readVector(std::string path) {
  FILE *f = fopen(path.c_str(), "r");
  int m, n;
  MM_typecode matcode;
  if (mm_read_banner(f, &matcode) != 0) {
    std::cout << "Error reading matrix banner" << std::endl;
    exit(1);
  }
  if (!mm_is_array(matcode)) {
    std::cout << "Expecting array in " << path << std::endl;
    exit(1);
  }
  mm_read_mtx_array_size(f, &m, &n);
  if (n != 1) {
    std::cout << "RHS should be column vector in " << path << std::endl;
    exit(1);
  }

  std::vector<double> v(m);
  for (int i = 0; i < m; i++)
    fscanf(f, "%lf", &v[i]);

  fclose(f);
  return v;
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
