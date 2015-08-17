#ifndef IO_HPP_QPBO8W0X
#define IO_HPP_QPBO8W0X

#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <stdexcept>
#include <fstream>

namespace spark {
  namespace io {

    using CoordType = std::tuple<int, int, double>;
    using CoordinateType = std::vector<CoordType>;

    CoordType mmparse(const std::string line) {
      std::stringstream ss{line};
      int x, y;
      double val;
      ss >> x >> y >> val;
      return std::make_tuple(x - 1, y - 1, val);
    }

    // Returns
    // NOTE the values will be adjusted to 0 based indexing
    // NOTE for symmetric matrices, the symmetric entries are also included
    CoordinateType mmread(std::string path) {
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
      CoordinateType values;
      while (getline(f, line)) {
        auto tpl = mmparse(line);
        values.push_back(tpl);
        if (isSymmetric)
          values.push_back(
              std::make_tuple(
                std::get<1>(tpl),
                std::get<0>(tpl),
                std::get<2>(tpl)));
      }

      return values;
    }
  }
}



#endif /* end of include guard: IO_HPP_QPBO8W0X */
