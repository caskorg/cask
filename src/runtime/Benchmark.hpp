#ifndef SPARSEBENCH_BENCHMARKUTILS_HPP
#define SPARSEBENCH_BENCHMARKUTILS_HPP

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

namespace cask {
namespace benchmark {

void checkFileExists(std::string file) {
  std::ifstream f{file};
  if (!f.good())
    throw std::invalid_argument("File does not exists " + file);
}

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

  // check files exists
  checkFileExists(argv[2]);
  checkFileExists(argv[4]);
  checkFileExists(argv[6]);
}



template<typename T>
std::string json(std::string key, T value, bool comma=true) {
  std::stringstream ss;
  ss << "\"" << key << "\":" << "\"" << value << "\"";
  if (comma)
    ss << ",";
  return ss.str();
}

void printSummary(
    double setupSeconds,
    int iterations,
    double solveSeconds,
    double estimatedError,
    double solutionVersusExpNorm,
    double benchmarkRepetitions,
    std::ostream& os=std::cout
) {
  os << "{"
     << json("setup took", setupSeconds)
     << json("iterations", iterations)
     << json("solve took", solveSeconds)
     << json("estimated error", estimatedError)
     << json("error", solutionVersusExpNorm)
     << json("bench repetitions", benchmarkRepetitions, false)
     << "}";
}

}
}

#endif //SPARSEBENCH_BENCHMARKUTILS_HPP
