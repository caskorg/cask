#include <Spark/Dse.hpp>
#include <Spark/Utils.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <string>

using namespace spark::utils;

int main(int argc, char** argv) {

  namespace po = boost::program_options;

  std::string numPipes = "numPipes";
  std::string cacheSize = "cacheSize";
  std::string inputWidth = "inputWidth";
  std::string gflopsOnly = "gflopsOnly";
  std::string help = "help";
  std::string help_m = "produce help message";

  po::options_description desc("Allowed options");
  desc.add_options()
    (help.c_str(), help_m.c_str())
    (numPipes.c_str(), po::value<std::string>(), "range for number of pipes: start,end,step")
    (cacheSize.c_str(), po::value<std::string>(), "range for cache size")
    (inputWidth.c_str(), po::value<std::string>(), "range for input width")
    (gflopsOnly.c_str(), po::value<bool>(), "print only gflops")
    ("filePath", po::value<std::string>(), "path to matrix")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  std::string path;
  if (vm.count("filePath")) {
    path = vm["filePath"].as<std::string>();
  } else {
    std::cout << "Matrix path was not set.\n";
    return 1;
  }

  Range numPipesRange{1, 4, 1}, inputWidthRange{8, 40, 8}, cacheSizeRange{1024, 1024 * 32, 1024};

  if (vm.count(numPipes)) {
    numPipesRange = Range(vm[numPipes].as<std::string>());
  }
  if (vm.count(inputWidth)) {
    inputWidthRange = Range(vm[inputWidth].as<std::string>());
  }
  if (vm.count(cacheSize)) {
     cacheSizeRange = Range(vm[cacheSize].as<std::string>());
  }

  bool gflopsOnlyV = false;
  if (vm.count(gflopsOnly)) {
    gflopsOnlyV = vm[gflopsOnly].as<bool>();
  }

  auto start = std::chrono::high_resolution_clock::now();

  spark::dse::SparkDse dseTool;
  spark::dse::DseParameters params;
  params.numPipesRange = numPipesRange;
  params.inputWidthRange = inputWidthRange;
  params.cacheSizeRange = cacheSizeRange;
  params.gflopsOnly = true;

  spark::dse::Benchmark benchmark;
  benchmark.add_matrix_path(path);
  dseTool.run(benchmark, params);
  dfesnippets::timing::print_clock_diff("Test took: ", start);
  return 0;
}
