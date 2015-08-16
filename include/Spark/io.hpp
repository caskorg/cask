#ifndef IO_HPP_QPBO8W0X
#define IO_HPP_QPBO8W0X

#include <vector>
#include <utility>
#include <string>
#include <stdexcept>
#include <fstream>

namespace spark {
  namespace io {

    std::vector<std::tuple<int, int, double>> mmread(std::string path) {
      std::ifstream f{path};
      if (!f.is_open())
        throw std::invalid_argument("Could not open file path " + path);
      string line;

    }

  }
}



#endif /* end of include guard: IO_HPP_QPBO8W0X */
