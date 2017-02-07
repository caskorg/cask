#ifndef TIMER_H
#define TIMER_H

#include <string>
#include <chrono>
#include <map>
#include <stdexcept>

namespace spam {

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
#endif //TIMER_H
