#ifndef IO_HPP_QPBO8W0X
#define IO_HPP_QPBO8W0X

#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <stdexcept>
#include <fstream>

#include <Spark/SparseMatrix.hpp>

namespace spark {
  namespace io {

    template<typename value_type>
      class MmReader {

        using SparseMatrix = spark::sparse::SparkCooMatrix<value_type>;
        using CoordType = typename SparseMatrix::CoordType;

        typename SparseMatrix::CoordType mmparse(const std::string line) {
          std::stringstream ss{line};
          int x, y;
          value_type val;
          ss >> x >> y >> val;
          return std::make_tuple(x - 1, y - 1, val);
        }

        public:

        MmReader() {}

        // Read the given file as a MM format description of a sparse matrix.
        // Returns a COO representation of the matrix.
        // NOTE:
        // - the values will be adjusted to 0 based indexing
        // - for symmetric matrices, the symmetric entries are also included
        // - Triplets will be sorted in lexicographical order of their coordinates (row, column)
        spark::sparse::SparkCooMatrix<value_type> mmread(std::string path) {
          std::ifstream f{path};
          if (!f.is_open())
            throw std::invalid_argument("Could not open file path " + path);
          std::string line;

          // handle header
          bool isSymmetric = true;
          if (getline(f, line)) {
            if (line[0] =='%' && line[1] == '%') {
              if (line.find("matrix") == std::string::npos) {
                isSymmetric = line.find("symmetric") != std::string::npos;
              }
            }
            std::cout << "Header " << line  << std::endl;
          } else
            throw std::invalid_argument("File " + path + " is empty");

          // skip comments
          while (getline(f, line) && line[0] == '%')
            continue;

          // read the dimensions
          int ncols, nrows, nnzs;
          std::stringstream ss;
          ss << line;
          ss >> nrows >> ncols >> nnzs;

          // skip comments
          std::vector<CoordType> values;
          while (getline(f, line)) {
            auto tpl = mmparse(line);
            values.push_back(tpl);
            auto row = std::get<0>(tpl);
            auto col = std::get<1>(tpl);
            if (isSymmetric && row != col)
              values.push_back(
                  std::make_tuple(
                    std::get<1>(tpl),
                    std::get<0>(tpl),
                    std::get<2>(tpl)));
          }

          std::sort(values.begin(), values.end(),
              [](CoordType x, CoordType y) {
              return
              std::get<0>(x) < std::get<0>(y) ||
              (std::get<0>(x) == std::get<0>(y) && std::get<1>(x) < std::get<1>(y));
              });

          spark::sparse::SparkCooMatrix<value_type> m(nrows, ncols);
          m.data = values;
          return m;
        }
      };
  }
}



#endif /* end of include guard: IO_HPP_QPBO8W0X */
