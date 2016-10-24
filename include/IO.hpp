#ifndef SPARSEBENCH_IO_HPP
#define SPARSEBENCH_IO_HPP

#include <string>
#include <iostream>
#include <SparseMatrix.hpp>

extern "C" {
# include <mmio.h>
#include <common.h>
};

namespace spam {
namespace io {

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
