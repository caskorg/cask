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

template<typename T>
void align(std::vector<T>& v, int widthInBytes) {
  int limit = widthInBytes / sizeof(T);
  while ((v.size() * sizeof(T)) % widthInBytes != 0 && limit != 0) {
    v.push_back(T{});
    limit--;
  }
}

inline int align(int bytes, int to) {
  int quot = bytes / to;
  if (bytes % to != 0)
    return (quot + 1) * to;
  return bytes;
}

template<typename T>
long size_bytes(const std::vector<T>& v) {
  return sizeof(T) * v.size();
}

inline int ceilDivide(int a, int b) {
  if (a < 0 || b < 0)
    throw std::invalid_argument("ceilDivide: arguments must be positive");
  return a / b + (a % b != 0);
}

template<typename Arg, typename... Args>
void logResultR(std::string s, Arg a, Args... as) {
  logResultR(s, as...);
  std::cout << a << ",";
}

inline void logResultR(std::string s) {
  std::cout << "Result " << " ";
  std::cout << s << "=";
}

template<typename U>
void logResult(std::string s, std::vector<U> vals) {
  logResultR(s);
  for (const auto& v : vals)
    std::cout << v << ",";
  std::cout << std::endl;
}

template<typename Arg, typename... Args>
void logResult(std::string s, Arg a, Args... as) {
  logResultR(s, as...);
  std::cout << a << ",";
  std::cout << std::endl;
}

// A ranged design parameter; it is defined with a fixed value OR a range
template<typename T=int>
class Parameter {
  Parameter(std::string _name, T _start, T _end, T _step, T _value) :
      name(_name), start(_start), end(_end), step(_step), value(_value) {}

 public:
  T start, end, step;
  T value;
  std::string name;
  Parameter(std::string _name, T _start, T _end, T _step) :
    Parameter(_name, _start, _end, _step, _start) {}

  // Construct a parameter with a single element range
  Parameter(std::string _name, T _start) :
      Parameter(_name, _start, _start, 1, _start) {}

  Parameter first() {
    return Parameter(name, start, end, step, start);
  }

  Parameter next() {
    if (value + step > end)
      throw std::invalid_argument("Invalid call to next() - no more elements");
    return Parameter(name, start, end, step, value + step);
  }

  Parameter last() {
    return Parameter(name, start, end, step, end);
  }

  bool hasNext() {
    return value != last().value;
  }

};

template<typename T>
inline std::ostream& operator<<(std::ostream& s, const Parameter<T>& p) {
  std::cout << "Parameter{";
  std::cout << p.start << "," << p.end << "," << p.step << "}";
  return s;
}

template<typename T=int>
class ChainedParameterRange {

  std::vector<Parameter<T>> range;

 public:
  ChainedParameterRange(std::vector<Parameter<T>> _range) : range(_range) {}
  ChainedParameterRange(std::initializer_list<Parameter<T>> _range) : range(_range) {}

  void start() {
    for (int i = 0; i < range.size(); i++) {
      range[i] = range[i].first();
    }
  }

  bool hasNext() {
    for (auto&& p : range) {
      if (p.hasNext())
        return true;
    }
    return false;
  }

  void next() {
    int i = 0;
    for (; i < range.size(); i++) {
      if (range[i].hasNext())
        break;
      range[i] = range[i].first();
    }
    if (i == range.size()) {
      throw std::invalid_argument("No next element available");
    }
    range[i] = range[i].next();
  }

  Parameter<T> getParam(std::string name) {
    for (auto& p : range) {
      if (p.name == name)
        return p;
    }
    throw std::invalid_argument("Param not found " + name);
  }

};

}
}

#endif /* end of include guard: UTILS_H */
