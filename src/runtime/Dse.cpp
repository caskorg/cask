#include <iostream>
#include "Dse.hpp"
#include <unordered_map>
#include "Converters.hpp"

using namespace cask::spmv;
using namespace cask::utils;
using namespace cask::model;
using namespace cask::dse;


std::shared_ptr<Spmv>
better(
    std::shared_ptr<Spmv> a1,
    std::shared_ptr<Spmv> a2) {

  if (a1 == nullptr)
    return a2;

  bool a1Better =
    a1->getEstimatedGFlops() > a2->getEstimatedGFlops() ||
    (a1->getEstimatedGFlops() == a2->getEstimatedGFlops() &&
     a1->getImplementationParameters().ru < a2->getImplementationParameters().ru);

  if (a1Better)
    return a1;
  return a2;
}

std::shared_ptr<Spmv> dse_run(
    std::string basename,
    SpmvArchitectureSpace* af,
    const Eigen::SparseMatrix<double, Eigen::RowMajor>& mat,
    const DseParameters& params)
{
  int it = 0;

  // for virtex 6
  double alpha = 0.9; // aim to fit about alpha% of the chip
  const LogicResourceUsage maxResources(LogicResourceUsage{297600, 297600, 1064, 2016} * alpha);

  const ImplementationParameters maxParams{maxResources, 39};

  std::shared_ptr<Spmv> bestArchitecture, a;

  while (a = af->next()) {
    auto start = std::chrono::high_resolution_clock::now();
    a->preprocess(mat); // do spmv?
    //dfesnippets::timing::print_clock_diff("Took: ", start);
    if (!(a->getImplementationParameters() < maxParams))
      continue;

    std::cout << basename << " " << a->to_string() << " " << a->getImplementationParameters().to_string() << std::endl;
    bestArchitecture = better(bestArchitecture, a);
  }

  if (!bestArchitecture)
    return nullptr;

  std::cout << basename << " ";
  if (params.gflopsOnly) {
    std::cout << bestArchitecture->getEstimatedGFlops();
    std::cout << bestArchitecture->getEstimatedClockCycles();
  } else {
    std::cout << bestArchitecture->to_string();
    std::cout << " " << bestArchitecture->getImplementationParameters().to_string();
  }
  std::cout << " Best " << std::endl;
  return bestArchitecture;
}

struct SpmvHash {
  std::size_t operator()(const std::shared_ptr<Spmv>& a) const {
    return 1;
  }
};

struct SpmvEqual {
  bool operator()(
      const std::shared_ptr<Spmv>& a,
      const std::shared_ptr<Spmv>& b) const
  {
    return *a == *b;
  }
};

std::vector<DseResult> cask::dse::SparkDse::run (
    const Benchmark& benchmark,
    const cask::dse::DseParameters& params)
{

  std::vector<DseResult> bestArchitectures;

  std::unordered_map<
    std::shared_ptr<Spmv>,
    std::vector<std::string>,
    SpmvHash,
    SpmvEqual
    > all_architectures;

  for (int i = 0; i < benchmark.get_benchmark_size(); i++) {

    std::string path = benchmark.get_matrix_path(i);

    std::size_t pos = path.find_last_of("/");
    std::string basename(path.substr(pos, path.size() - pos));
    std::cout << basename << std::endl;

    cask::io::MmReader<double> m(path);
    auto start = std::chrono::high_resolution_clock::now();
    auto eigenMatrix = cask::converters::tripletToEigen(m.mmreadMatrix(path));
    dfesnippets::timing::print_clock_diff("Reading took: ", start);

    // XXX this assumes a virtex device with 512 entries per BRAM
    int maxRows = eigenMatrix->rows();
    if (maxRows % 512 != 0)
      maxRows = (maxRows / 512 + 1) * 512;

    std::vector<SpmvArchitectureSpace*> factories{
      new SimpleSpmvArchitectureSpace<BasicSpmv>(
          params.numPipesRange, params.inputWidthRange, params.cacheSizeRange, maxRows),
          //new SimpleSpmvArchitectureSpace<FstSpmv>(
              //params.numPipesRange, params.inputWidthRange, params.cacheSizeRange),
          //new SimpleSpmvArchitectureSpace<SkipEmptyRowsSpmv>(
              //params.numPipesRange, params.inputWidthRange, params.cacheSizeRange),
          //new SimpleSpmvArchitectureSpace<PrefetchingArchitecture>(numPipesRange, inputWidthRange, cacheSizeRange)
    };

    std::cout << "File Architecture CacheSize InputWidth NumPipes EstClockCycles EstGflops LUTS FFs DSPs BRAMs MemBandwidth Observation" << std::endl;
    std::shared_ptr<Spmv> bestOverall;
    for (auto sas : factories) {
      bestOverall = better(
          bestOverall,
          dse_run(basename, sas, *eigenMatrix, params));
    }

    if (!bestOverall)
      continue;

    for (int i = 0; i < factories.size(); i++)
      delete(factories[i]);

    std::cout  << basename << " ";
    if (params.gflopsOnly) {
      std::cout << bestOverall->getEstimatedGFlops();
      std::cout << bestOverall->getEstimatedClockCycles();
    } else {
      std::cout << bestOverall->to_string();
      std::cout << " "  << bestOverall->getImplementationParameters().to_string();
    }
    std::cout << " BestOverall " << std::endl;


    //auto a = all_architectures.find(bestOverall);
    //if ( a != all_architectures.end()) {
      //a->second.push_back(path);
    //} else {
      //all_architectures.insert(
          //std::make_pair(bestOverall, std::vector<std::string>{path}));
    //}

    bestArchitectures.push_back(DseResult{path, bestOverall});
  }

  //for (const auto& a : all_architectures) {
    //DseResult result{a.first};
    //result.matrices = a.second;
    //bestArchitectures.push_back(result);
  //}

  return bestArchitectures;
}
