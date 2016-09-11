#ifndef SPARSEBENCH_BENCHMARKUTILS_HPP
#define SPARSEBENCH_BENCHMARKUTILS_HPP

#include <stdexcept>
#include <sstream>
#include <iostream>

namespace sparsebench {
namespace benchmarkutils {

void parseArgs(int argc, char** argv) {
  if (argc != 3) {
    std::stringstream ss;
    ss << "Usage " << argv[0] << " <matrix> <rhs>" <<std::endl;
    throw new std::invalid_argument(ss.str());
  }
}

void printSummary(
    double setupSeconds,
    int iterations,
    double solveSeconds
) {
  std::cout << "setup took       " << setupSeconds << std::endl;
  std::cout << "#iterations:     " << iterations << std::endl;
  std::cout << "Solve took       " << solveSeconds << std::endl;
}

}
}

#endif //SPARSEBENCH_BENCHMARKUTILS_HPP
