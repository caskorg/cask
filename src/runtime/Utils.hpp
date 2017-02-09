#ifndef UTILS_HPP_DASDASD
#define UTILS_HPP_DASDASD

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

namespace cask {
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

class Timer {

  using clock_t = std::chrono::high_resolution_clock;
  using time_t = std::chrono::time_point<clock_t>;
  using duration_t = std::chrono::duration<double>;

  std::map<std::string, time_t> activeTimers;
  std::map<std::string, duration_t> durations;

 public:

  // the time between this call and the next toc with the same key will be recorded;
  // can be retrieved using the key; if the same key is passed multiple times only the last
  // time is recorded
  void tic(std::string name) {
    activeTimers[name] = clock_t::now();
  }

  // reset timer and return time elapsed from previous tic
  duration_t toc(std::string name) {
    duration_t p;
    if (activeTimers.find(name) != activeTimers.end()) {
      durations[name] = clock_t::now() - activeTimers[name];
      activeTimers.erase(name);
      return durations[name];
    }
    throw std::invalid_argument("No previous tic() with " + name);
  }

  duration_t get(std::string name) {
    if (durations.find(name) == durations.end())
      throw std::invalid_argument("No previous tic()/toc() with " + name);
    return durations[name];
  }

};

template<typename T>
void print(T v, std::string message="") {
  std::cout << message;
  for (typename T::value_type t : v) {
    std::cout << t << " ";
  }
  std::cout << std::endl;
}

  }
}

#endif /* end of include guard: UTILS_H */
