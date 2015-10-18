#ifndef SPARK_H
#define SPARK_H

#include <iostream>
#include <Spark/Dse.hpp>

using namespace spark::spmv;
using namespace spark::utils;
using namespace spark::model;
using namespace spark::dse;

void spark::dse::SparkDse::runDse() {
  std::cout << "Running DSE" << std::endl;
}

std::shared_ptr<SpmvArchitecture> dse_run(
    std::string basename,
    SpmvArchitectureSpace* af,
    const Eigen::SparseMatrix<double, Eigen::RowMajor>& mat,
    const DseParameters& params)
{
  int it = 0;

  // for virtex 6
  double alpha = 0.9; // aim to fit about 90% of the chip
  const LogicResourceUsage maxResources(LogicResourceUsage{297600, 297600, 1064, 2016} * alpha);

  const ImplementationParameters maxParams{maxResources, 39};

  std::shared_ptr<SpmvArchitecture> bestArchitecture, a;

  while (a = af->next()) {
    auto start = std::chrono::high_resolution_clock::now();
    a->preprocess(mat); // do spmv?
    //dfesnippets::timing::print_clock_diff("Took: ", start);
    if (!(a->getImplementationParameters() < maxParams))
      continue;

    std::cout << basename << " " << a->to_string() << " " << a->getImplementationParameters().to_string() << std::endl;
    if (bestArchitecture == nullptr ||
        *a < *bestArchitecture) {
      bestArchitecture = a;
    }
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

int spark::dse::SparkDse::run (
    const Benchmark& benchmark,
    const spark::dse::DseParameters& params)
{

  std::string path = benchmark.get_matrix_path(0);

  std::size_t pos = path.find_last_of("/");
  std::string basename(path.substr(pos, path.size() - pos));
  std::cout << basename << std::endl;

  spark::io::MmReader<double> m(path);
  auto start = std::chrono::high_resolution_clock::now();
  auto eigenMatrix = spark::converters::tripletToEigen(m.mmreadMatrix(path));
  dfesnippets::timing::print_clock_diff("Reading took: ", start);

  // XXX memory leak
  std::vector<SpmvArchitectureSpace*> factories{
    new SimpleSpmvArchitectureSpace<SimpleSpmvArchitecture>(
        params.numPipesRange, params.inputWidthRange, params.cacheSizeRange),
        new SimpleSpmvArchitectureSpace<FstSpmvArchitecture>(
            params.numPipesRange, params.inputWidthRange, params.cacheSizeRange),
        new SimpleSpmvArchitectureSpace<SkipEmptyRowsArchitecture>(
            params.numPipesRange, params.inputWidthRange, params.cacheSizeRange),
        //new SimpleSpmvArchitectureSpace<PrefetchingArchitecture>(numPipesRange, inputWidthRange, cacheSizeRange)
  };

  std::cout << "File Architecture CacheSize InputWidth NumPipes EstClockCycles EstGflops LUTS FFs DSPs BRAMs MemBandwidth Observation" << std::endl;
  std::shared_ptr<SpmvArchitecture> bestOverall;
  for (auto sas : factories) {
    std::shared_ptr<SpmvArchitecture> best = dse_run(basename, sas, *eigenMatrix, params);
    if (!bestOverall) {
      bestOverall = best;
    } else {
      if (best && *best < *bestOverall) {
        bestOverall = best;
      }
    }
  }

  for (int i = 0; i < factories.size(); i++)
    delete(factories[i]);

  if (!bestOverall)
    return 1;

  std::cout  << basename << " ";
  if (params.gflopsOnly) {
    std::cout << bestOverall->getEstimatedGFlops();
    std::cout << bestOverall->getEstimatedClockCycles();
  } else {
    std::cout << bestOverall->to_string();
    std::cout << " "  << bestOverall->getImplementationParameters().to_string();
  }
  std::cout << " BestOverall " << std::endl;
}

#endif /* end of include guard: SPARK_H */
