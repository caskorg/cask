#include "Spmv.hpp"
#include "Converters.hpp"
#include <iostream>
#include <tuple>
#include <dfesnippets/VectorUtils.hpp>
#include <dfesnippets/Timing.hpp>
#include <cassert>

#include "GeneratedImplSupport.hpp"
#include "Utils.hpp"

using namespace cask::spmv;
using ssarch = cask::spmv::Spmv;
namespace cutils = cask::utils;

struct PartitionWriteResult {
  int64_t outStartAddr, outSize, colptrStartAddress, colptrSize;
  int64_t vStartAddress, indptrValuesStartAddress, indptrValuesSize;
};

template<typename T>
std::vector<T> msinglearray(int size, int pos, T value) {
  std::vector<T> v(size);
  v[pos] = value;
  return v;
}

// write the data for a partition, starting at the given offset
PartitionWriteResult writeDataForPartition(
    const cask::runtime::GeneratedSpmvImplementation& impl,
    int offset,
    const cask::spmv::Partition& br,
    const std::vector<double>& v,
    int numControllers,
    int controllerNum) {
  // for each partition write this down
  PartitionWriteResult pwr;
  pwr.indptrValuesStartAddress = cutils::align(offset, 384);
  pwr.indptrValuesSize = cutils::size_bytes(br.m_indptr_values);
  // std::vector<int64_t> sizes = {pwr.indptrValuesSize};
  // std::vector<int64_t> addrs = {pwr.indptrValuesStartAddress};
  auto sizes = msinglearray(numControllers, controllerNum, pwr.indptrValuesSize);
  auto addrs = msinglearray(numControllers, controllerNum, pwr.indptrValuesStartAddress);
  // std::vector<const indptr_value*> data = {&br.m_indptr_values[0]};
  // std::cout << "Writing to controller" << controllerNum << std::endl;
  // std::cout << "Write 1 " << std::endl;
  // cutils::print(sizes, "sizes=");
  // cutils::print(addrs, "addrs=");
  // std::cout << "indptr_values=";
  //for (auto t : br.m_indptr_values) {
    //std::cout << t.indptr <<"," << t.value << " ";
  //}

  std::string routingString = "split -> tomem" + std::to_string(controllerNum);
  impl.deviceInterface.write(
      sizes[controllerNum],
      &sizes[0],
      &addrs[0],
      (uint8_t*)&br.m_indptr_values[0],
      routingString.c_str());

  pwr.vStartAddress = pwr.indptrValuesStartAddress + pwr.indptrValuesSize;
  sizes = msinglearray(numControllers, controllerNum, cutils::size_bytes(v));
  addrs = msinglearray(numControllers, controllerNum, pwr.vStartAddress);
  // std::cout << "Write 2 " << std::endl;
  // cutils::print(sizes, "sizes=");
  // cutils::print(addrs, "addrs=");
  // data  = msinglearray(numControllers, controllerNum, &v[0]);
  impl.deviceInterface.write(
      sizes[controllerNum],
      &sizes[0],
      &addrs[0],
      (uint8_t*)&v[0],
      routingString.c_str());

  // std::cout << "Write 3 " << std::endl;
  // data = msinglearray(numControllers, controllerNum, &br.m_colptr[0]);
  pwr.colptrStartAddress = pwr.vStartAddress + cutils::size_bytes(v);
  pwr.colptrSize = cutils::size_bytes(br.m_colptr);
  sizes = msinglearray(numControllers, controllerNum, pwr.colptrSize);
  addrs = msinglearray(numControllers, controllerNum, pwr.colptrStartAddress);
  // cutils::print(sizes, "sizes=");
  // cutils::print(addrs, "addrs=");
  // cutils::print(br.m_colptr, "colptr=");
  impl.deviceInterface.write(
      sizes[controllerNum],
      &sizes[0],
      &addrs[0],
      (uint8_t*)&br.m_colptr[0],
      routingString.c_str());

  pwr.outStartAddr = pwr.colptrStartAddress + pwr.colptrSize;
  pwr.outSize = br.outSize;
  return pwr;
}

// how many cycles does it take to resolve the accesses
int ssarch::countComputeCycles(int32_t* v, int size, int inputWidth)
{
  int cycles = 0;
  int crtPos = 0;
  for (int i = 0; i < size; i++) {
    int toread = v[i] - (i > 0 ? v[i - 1] : 0);
    do {
      int canread = std::min(inputWidth - crtPos, toread);
      crtPos += canread;
      crtPos %= inputWidth;
      cycles++;
      toread -= canread;
    } while (toread > 0);
  }
  return cycles;
}
// transform a given matrix with n rows in blocks of size n X blockSize
Partition ssarch::do_blocking(
    const CsrMatrix& m,
    int blockSize,
    int inputWidth)
{
  int rows = m.n;
  int cols = m.m;
  int n = rows;
  //std::cout << "Mat rows " << n << std::endl;
  //std::cout << "Npartitions: " << nPartitions << std::endl;

  std::vector<CsrMatrix> partitions = m.sliceColumns(blockSize); // (nBlocks);
  int nBlocks = partitions.size();

  //if (n > Spmv_maxRows)
    //throw std::invalid_argument(
        //"Matrix has too many rows - maximum supported: "
        //+ std::to_string(Spmv_maxRows));

  std::vector<double> v(cols, 0);
  std::vector<int> m_colptr;
  std::vector<indptr_value> m_indptr_value;

  // now we coalesce partitions
  int cycles = 0;
  int partition = 0;
  int reductionCycles = rows * partitions.size();
  int emptyCycles = 0;
  for (auto& p : partitions) {
    auto pp = preprocessBlock(p, partition++, partitions.size());
    auto& p_colptr = pp.row_ptr;
    auto& p_indptr = pp.col_ind;
    auto& p_values = pp.values;

    int diff = p.row_ptr.size() - p_colptr.size();
    emptyCycles += diff;
    reductionCycles -= diff;
    cycles += this->countComputeCycles(&p.row_ptr[0], n, inputWidth) - diff;

    utils::align(p_indptr, sizeof(int) * inputWidth);
    utils::align(p_values, sizeof(double) * inputWidth);
    copy(p_colptr.begin(), p_colptr.end(), back_inserter(m_colptr));
    for (size_t i = 0; i < p_values.size(); i++)
      m_indptr_value.push_back(indptr_value( p_values[i], p_indptr[i]));
  }

  Partition br;
  br.m_colptr_unpaddedLength = m_colptr.size();
  br.m_indptr_values_unpaddedLength = m_indptr_value.size();
  //std::cout << "m_colptr unaligned size" << m_colptr.size() << std::endl;
  utils::align(m_colptr, 384);
  utils::align(m_indptr_value, 384);
  std::vector<double> out(n, 0);
  utils::align(out, 384);
  utils::align(v, sizeof(double) * blockSize);

  br.nBlocks = nBlocks;
  br.n = n;
  br.paddingCycles = out.size() - n; // number of cycles required to align to the burst size
  br.totalCycles = cycles + v.size();
  br.vector_load_cycles = v.size() / nBlocks; // per partition
  br.m_indptr_values = m_indptr_value;
  br.m_colptr = m_colptr;
  br.outSize = out.size() * sizeof(double);
  br.emptyCycles = emptyCycles;
  br.reductionCycles = reductionCycles;

  return br;
}

cask::Vector ssarch::spmv(const cask::Vector& x)
{
  using namespace std;

  if (this->impl.params.dram_reduction_enabled) {
    // because of DRAM read/write latency we can only hope to get correct
    // answers for large matrices
    // XXX figure out where to place this constant;
    const int minRowsWithDramReduction = 35000;
    if (mat.n < minRowsWithDramReduction ) {
      stringstream ss;
      ss << "Matrix is too small! Minimum supported rows with DRAM reduction: ";
      ss << minRowsWithDramReduction;
      ss << " actual rows: " << mat.n;
      throw invalid_argument(ss.str());
    }
  } else if (impl.params.max_rows < mat.n) {
      stringstream ss;
      ss << "Matrix is too large! Maximum supported rows: ";
      ss << impl.params.max_rows;
      ss << " actual rows: " << mat.n;
      throw invalid_argument(ss.str());
  }

  int cacheSize = impl.params.cache_size;

  vector<double> v = x.data;
  utils::align(v, sizeof(double) * cacheSize);
  utils::align(v, 384);

  vector<int> nrows, totalCycles, reductionCycles, paddingCycles, colptrSizes, indptrValuesSizes, outputResultSizes;
  vector<int> colptrUnpaddedSizes, indptrValuesUnpaddedLengths;
  vector<long> outputStartAddresses, colptrStartAddresses;
  vector<long> vStartAddresses, indptrValuesStartAddresses;

  int offset = 0;
  int i = 0;

  assert(partitions.size() == impl.params.num_pipes && "numPipes should equal numPartitions");
  assert(impl.params.num_pipes % impl.params.num_controllers == 0 && "numPipes should be a multiple of numControllers");
  assert(impl.params.num_controllers <= impl.params.num_pipes && "numPipes should be larger than numControllers");
  for (auto& p : partitions) {
    nrows.push_back(p.n);
    paddingCycles.push_back(p.paddingCycles);
    totalCycles.push_back(p.totalCycles);
    reductionCycles.push_back(p.reductionCycles);
    colptrUnpaddedSizes.push_back(p.m_colptr_unpaddedLength);
    indptrValuesUnpaddedLengths.push_back(p.m_indptr_values_unpaddedLength);

    int nc = impl.params.num_controllers;
    int pipesPerController = impl.params.num_pipes / nc;
    int ctrlId = i / pipesPerController;
    // moving to a new controller, reset offset in memory
    if (i % pipesPerController == 0) {
      offset = 0;
    }
    PartitionWriteResult pr = writeDataForPartition(this->impl, offset, p, v, nc, ctrlId);
    outputStartAddresses.push_back(pr.outStartAddr);
    outputResultSizes.push_back(pr.outSize);
    colptrStartAddresses.push_back(pr.colptrStartAddress);
    colptrSizes.push_back(pr.colptrSize);
    vStartAddresses.push_back(pr.vStartAddress);
    indptrValuesSizes.push_back(pr.indptrValuesSize);
    indptrValuesStartAddresses.push_back(pr.indptrValuesStartAddress);

    offset = pr.outStartAddr + p.outSize;
    i++;
  }

  // npartitions and vector load cycles should be the same for all partitions
  int nBlocks = this->partitions[0].nBlocks;
  int vector_load_cycles = this->partitions[0].vector_load_cycles;
  cout << "Running on DFE" << endl;

  int nIterations = 1;
  utils::logResult("Total cycles", totalCycles);
  utils::logResult("Padding cycles", paddingCycles);
  utils::logResult("Reduction cycles", reductionCycles);

  auto start = chrono::_V2::system_clock::now();
  impl.deviceInterface.run(
      nIterations,
      nBlocks,
      vector_load_cycles,
      &colptrStartAddresses[0],
      &colptrSizes[0],
      &indptrValuesStartAddresses[0],
      &indptrValuesSizes[0],
      &nrows[0],
      &outputStartAddresses[0],
      &reductionCycles[0],
      &totalCycles[0],
      &vStartAddresses[0]
      );
  double took = dfesnippets::timing::clock_diff(start) / nIterations;
  double est =(double) totalCycles[0] / getFrequency();
  double gflopsEst = (2.0 * (double)this->mat.nnzs / est) / 1E9;
  double gflopsActual = (2.0 * (double)this->mat.nnzs / took) / 1E9;

  double bwidthEst = impl.params.num_pipes * impl.params.input_width * getFrequency() / (1024.0  *
      1024 * 1024) * (8 + 4);
  utils::logResult("Input width ", impl.params.input_width);
  utils::logResult("Pipes ", impl.params.num_pipes);

  utils::logResult("Iterations", nIterations);
  utils::logResult("Took (ms)", took);
  utils::logResult("Est (ms)", est);
  utils::logResult("Gflops (est)", gflopsEst);
  utils::logResult("Gflops (actual)", gflopsActual);
  utils::logResult("BWidth (est)", bwidthEst);

  vector<double> total;
  for (size_t i = 0; i < outputStartAddresses.size(); i++) {
    vector<double> tmp(outputResultSizes[i] / sizeof(double), 0);
    int ctrlId = i / (impl.params.num_pipes / impl.params.num_controllers);
    auto sizes = msinglearray(impl.params.num_controllers, ctrlId, utils::size_bytes(tmp));
    auto addrs = msinglearray(impl.params.num_controllers, ctrlId, outputStartAddresses[i]);
    string routing = "frommem" + std::to_string(ctrlId) + " -> join";
    impl.deviceInterface.read(
        sizes[ctrlId],
        &sizes[0],
        &addrs[0],
        (uint8_t*)&tmp[0],
        routing.c_str()
        );
    copy(tmp.begin(), tmp.begin() + nrows[i], back_inserter(total));
  }

  // remove the elements which were only for padding
  return Vector{total};
}
void ssarch::preprocess(
    const CsrMatrix& mat) {
  this->mat = mat;

  partitions.clear();
  int rowsPerPartition = mat.n / impl.params.num_pipes;
  int start = 0;
  for (int i = 0; i < impl.params.num_pipes - 1; i++) {
    this->partitions.push_back(
        do_blocking(
          mat.sliceRows(start, rowsPerPartition),
          impl.params.cache_size, impl.params.input_width));
    start += rowsPerPartition;
  }

  // put all rows left in the last partition
  this->partitions.push_back(
      do_blocking(mat.sliceRows(start, mat.n - start),
        impl.params.cache_size, impl.params.input_width));

}