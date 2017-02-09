#ifndef UTILS_HPP_DASDASD
#define UTILS_HPP_DASDASD

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace spark {
  namespace utils {
    struct Range {
      int start, end, step, crt;
      Range(int _s, int _e, int _stp) : start(_s), end(_e), step(_stp), crt(_s) {}
      Range(std::string range) {
        std::vector<std::string> strs;
        if (range.find(",") != std::string::npos) {
          // assume a (start, end, step) range format
          boost::split(strs, range, boost::is_any_of(","));
          start = std::stoi(strs[0]);
          end = std::stoi(strs[1]);
          step = std::stoi(strs[2]);
        } else {
          // assume a single element range (start, start + 1, 1)
          start = std::stoi(range);
          end = start + 1;
          step = 1;
        }

        crt = start;
      }

      std::string to_string() {
        std::stringstream ss;
        ss << start << ", " << end << ", " << step;
        return ss.str();
      }

      void restart() {
        crt = start;
      }

      bool at_start() {
        return crt == start;
      }

      int operator++() {
        crt = crt + step;
        if (crt == end) {
          crt = start;
        }
        return crt;
      }
    };

    inline std::ostream& operator<<(std::ostream& s, const Range& r) {
      std::cout << "Range{";
      std::cout << r.start << "," << r.end << "," << r.step << "}";
      return s;
    }
  }
}

#endif /* end of include guard: UTILS_H */
