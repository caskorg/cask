#include <string>
#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>
#include <Spark/Utils.hpp>

#include <Eigen/Sparse>
#include <Spark/io.hpp>
#include <dfesnippets/Timing.hpp>

#include <chrono>
#include <boost/program_options.hpp>

using namespace spark::spmv;
using namespace spark::utils;

struct Params {
  bool gflopsOnly;
  Params(bool _gflopsOnly) : gflopsOnly(_gflopsOnly) {}
};

// returns the best architecture
std::shared_ptr<SpmvArchitecture> dse(
    std::string basename,
    SpmvArchitectureSpace* af,
    Eigen::SparseMatrix<double, Eigen::RowMajor> mat,
    const Params& params) {
  int it = 0;

  std::shared_ptr<SpmvArchitecture> bestArchitecture, a;

  while (a = af->next()) {
    auto start = std::chrono::high_resolution_clock::now();
    a->preprocess(mat); // do spmv?
    std::cout << "Matrix: " << basename << " " << a->to_string();
    std::cout << " ResourceUsage: " << a->getResourceUsage().to_string() << std::endl;
    dfesnippets::timing::print_clock_diff("Took: ", start);
    if (bestArchitecture == nullptr ||
        a->getEstimatedGFlops() > bestArchitecture->getEstimatedGFlops()) {
      bestArchitecture = a;
    }
  }

  std::cout << "Best ";
  std::cout << "Mat: " << basename << " Arch: " << bestArchitecture->get_name();
  if (params.gflopsOnly) {
    std::cout << " est. gflops " << bestArchitecture->getEstimatedGFlops();
    std::cout << "est. cycles " << bestArchitecture->getEstimatedClockCycles() << std::endl;
  } else {
    std::cout << bestArchitecture->to_string() << std::endl;
    std::cout << " ResourceUsage: " << bestArchitecture->getResourceUsage().to_string() << std::endl;
  }
  return bestArchitecture;
}

int run (
    std::string path,
    Range numPipesRange, Range inputWidthRange, Range cacheSizeRange,
    const Params& params
    ) {

  std::size_t pos = path.find_last_of("/");
  std::string basename(path.substr(pos, path.size() - pos));
  std::cout << basename << std::endl;

  spark::io::MmReader<double> m(path);
  auto start = std::chrono::high_resolution_clock::now();
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  dfesnippets::timing::print_clock_diff("Reading took: ", start);

  // XXX memory leak
  std::vector<SpmvArchitectureSpace*> factories{
    new SimpleSpmvArchitectureSpace<SimpleSpmvArchitecture>(numPipesRange, inputWidthRange, cacheSizeRange),
    new SimpleSpmvArchitectureSpace<FstSpmvArchitecture>(numPipesRange, inputWidthRange, cacheSizeRange),
    new SimpleSpmvArchitectureSpace<SkipEmptyRowsArchitecture>(numPipesRange, inputWidthRange, cacheSizeRange)
  };

  for (auto sas : factories) {
    dse(basename, sas, *eigenMatrix, params);
  }

  delete(factories[0]);
  delete(factories[1]);
}


int main(int argc, char** argv) {

  namespace po = boost::program_options;

  std::string numPipes = "numPipes";
  std::string cacheSize = "cacheSize";
  std::string inputWidth = "inputWidth";
  std::string gflopsOnly = "gflopsOnly";

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
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

  Range numPipesRange{1, 6, 1}, inputWidthRange{8, 96, 8}, cacheSizeRange{1024, 4096, 1024};

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

  Params params{gflopsOnlyV};
  int status = run(path, numPipesRange, inputWidthRange, cacheSizeRange, params);

  dfesnippets::timing::print_clock_diff("Test took: ", start);
  if (status == 0)
    std::cout << "All tests passed!" << std::endl;
  else
    std::cout << "Tests failed!" << std::endl;
  return status;
}
