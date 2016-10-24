#ifndef SPARSEBENCH_IO_HPP
#define SPARSEBENCH_IO_HPP

#include <string>
#include <iostream>

extern "C" {
# include <mmio.h>
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

}
}

#endif //SPARSEBENCH_IO_HPP
