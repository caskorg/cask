#ifndef SPARSEBENCH_BENCHMARKUTILS_HPP
#define SPARSEBENCH_BENCHMARKUTILS_HPP

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

namespace sparsebench {
namespace benchmarkutils {

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
  // checkFileExists(argv[2]);
  // checkFileExists(argv[4]);
  // checkFileExists(argv[6]);
}

double residual(std::vector<double> got, std::vector<double> exp) {
  double residual = 0;
  for (int i = 0; i < got.size(); i++) {
    residual += (got[i] - exp[i]) * (got[i] - exp[i]);
  }
  return std::sqrt(residual);
}

void printSummary(
    double setupSeconds,
    int iterations,
    double solveSeconds,
    double estimatedError,
    double solutionVersusExpNorm,
    double benchmarkRepetitions
) {
  std::cout << "setup took       " << setupSeconds << std::endl;
  std::cout << "#iterations:     " << iterations << std::endl;
  std::cout << "Solve took       " << solveSeconds << std::endl;

  std::cout << "estimated error: " << estimatedError  << std::endl;
  std::cout << "Error vs expected: " << solutionVersusExpNorm<< std::endl;
  std::cout << "Benchmark repetions: " << benchmarkRepetitions << std::endl;
}

}
}

#endif //SPARSEBENCH_BENCHMARKUTILS_HPP
