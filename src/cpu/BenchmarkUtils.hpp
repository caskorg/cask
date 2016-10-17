#ifndef SPARSEBENCH_BENCHMARKUTILS_HPP
#define SPARSEBENCH_BENCHMARKUTILS_HPP

#include <stdexcept>
#include <sstream>
#include <iostream>

namespace sparsebench {
namespace benchmarkutils {

void parseArgs(int argc, char** argv) {
  // TODO may use a more flexible approach using boost::program_options
  if (argc != 7) {
    std::stringstream ss;
    ss << "Usage " << argv[0] << " -mat <matrix> -rhs <rhs> -lhs <lhs>" <<std::endl;
    throw std::invalid_argument(ss.str());
  }
  if (std::string(argv[1]) != "-mat") {
    throw std::invalid_argument("First argument should be -mat");
  }
  if (std::string(argv[3]) != "-rhs") {
    throw std::invalid_argument("Third argument should be -rhs");
  }
  if (std::string(argv[5]) != "-lhs") {
    throw std::invalid_argument("Fifth argument should be -lhs");
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
