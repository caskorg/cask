#include <iostream>
#include "Dse.hpp"
#include <unordered_map>
#include "Converters.hpp"
#include <Utils.hpp>

using namespace cask::spmv;
using namespace cask::utils;
using namespace cask::model;
using namespace cask::dse;

std::shared_ptr<Spmv>
better(
    std::shared_ptr<Spmv> a1,
    std::shared_ptr<Spmv> a2,
    const cask::model::DeviceModel& deviceModel,
    int matrixDimension) {

  if (a1 == nullptr)
    return a2;

  bool a1Better =
    a1->getEstimatedGFlops(deviceModel) > a2->getEstimatedGFlops(deviceModel) ||
    (a1->getEstimatedGFlops(deviceModel) == a2->getEstimatedGFlops(deviceModel) &&
     a1->getEstimatedHardwareModel(deviceModel, matrixDimension).ru < a2->getEstimatedHardwareModel(deviceModel, matrixDimension).ru);

  if (a1Better)
    return a1;
  return a2;
}

std::shared_ptr<Spmv> dse_run(
    std::string basename,
    ChainedParameterRange<int>& af,
    const cask::CsrMatrix& mat,
    const DseParameters& params,
    const cask::model::DeviceModel& deviceModel)
{
  std::shared_ptr<Spmv> bestArchitecture, a;
  while (af.hasNext()) {
    a = std::make_shared<cask::spmv::SkipEmptyRowsSpmv>(af.getParam("cacheSize").value,
                                           af.getParam("inputWidth").value,
                                           af.getParam("numPipes").value,
                                           af.getParam("maxRows").value,
                                           af.getParam("numControllers").value);

    af.next();
    auto start = std::chrono::high_resolution_clock::now();
    if (!a->isValid()) {
      continue;
    }
    if (!(a->getEstimatedHardwareModel(deviceModel, mat.n).ru < deviceModel.maxParams().ru)) {
      continue;
    }
    a->preprocess(mat); // do spmv?
    //dfesnippets::timing::print_clock_diff("Took: ", start);
    std::cout << basename << " " << a->to_string(deviceModel) << " " << a->getEstimatedHardwareModel(deviceModel, mat.n).to_string() << std::endl;
    bestArchitecture = better(bestArchitecture, a, deviceModel, mat.n);
  }

  if (!bestArchitecture)
    return nullptr;

  std::cout << basename << " ";
  if (params.gflopsOnly) {
    std::cout << bestArchitecture->getEstimatedGFlops(deviceModel);
    std::cout << bestArchitecture->getEstimatedClockCycles();
  } else {
    std::cout << bestArchitecture->to_string(deviceModel);
    std::cout << " " << bestArchitecture->getEstimatedHardwareModel(deviceModel, mat.n).to_string();
  }
  std::cout << " Best " << std::endl;
  return bestArchitecture;
}

std::vector<DseResult> cask::dse::SparkDse::run (
    const Benchmark& benchmark,
    const cask::dse::DseParameters& params,
    const cask::model::DeviceModel& deviceModel)
{

  std::vector<DseResult> bestArchitectures;

  for (int i = 0; i < benchmark.get_benchmark_size(); i++) {

    std::string path = benchmark.get_matrix_path(i);

    std::size_t pos = path.find_last_of("/");
    std::string basename(path.substr(pos, path.size() - pos));
    std::cout << basename << std::endl;

    cask::io::MmReader<double> m(path);
    auto start = std::chrono::high_resolution_clock::now();
    // auto eigenMatrix = cask::converters::tripletToEigen(m.mmreadMatrix(path));
    CsrMatrix matrix = cask::io::readMatrix(path);
    dfesnippets::timing::print_clock_diff("Reading took: ", start);

    // XXX this assumes a virtex device with 512 entries per BRAM
    int maxRows = matrix.n;
    if (maxRows % 512 != 0)
      maxRows = (maxRows / 512 + 1) * 512;

    ChainedParameterRange<int> cpr{
        params.numPipes,
        params.inputWidth,
        params.cacheSize,
        params.numControllers,
        Parameter<int>{"maxRows", maxRows, maxRows, 1}
    };

    std::cout << "File Architecture CacheSize InputWidth NumPipes EstClockCycles EstGflops LUTS FFs DSPs BRAMs MemBandwidth Observation" << std::endl;
    std::shared_ptr<Spmv> bestOverall = dse_run(basename, cpr, matrix, params, deviceModel);

    if (!bestOverall)
      continue;

    std::cout  << basename << " ";
    if (params.gflopsOnly) {
      std::cout << bestOverall->getEstimatedGFlops(deviceModel);
      std::cout << bestOverall->getEstimatedClockCycles();
    } else {
      std::cout << bestOverall->to_string(deviceModel);
      std::cout << " "  << bestOverall->getEstimatedHardwareModel(deviceModel, matrix.n).to_string();
    }
    std::cout << " BestOverall " << std::endl;
    std::cout << "Matrix: " << basename << " Best architecture: " << bestOverall->to_string(deviceModel) << " "  << bestOverall->getEstimatedHardwareModel(deviceModel, matrix.n).to_string() << std::endl;

    // do SpmvFor this architecture, to check the results for profiling
    cask::Vector lhs(matrix.n);
    bestOverall->preprocess(matrix);
    try {
      auto result = bestOverall->spmv(lhs);
    } catch (std::exception& e) {
      std::cout << "Could not run design " << e.what() << std::endl;
    }
    bestArchitectures.push_back(DseResult{path, bestOverall});
  }

  return bestArchitectures;
}
