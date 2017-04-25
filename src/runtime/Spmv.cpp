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

const int burst_size_bytes = 384;

struct PartitionWriteResult {
  int64_t outStartAddr, outSize, colptrStartAddress, colptrSize;
  int64_t vStartAddress, indptrValuesStartAddress, indptrValuesSize;
};

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
  std::vector<double> out(n, 0);
  cutils::align(out, burst_size_bytes);
  cutils::align(v, sizeof(double) * blockSize);

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

template<typename T>
std::vector<T> msinglearray(int size, int pos, T value) {
  std::vector<T> v(size);
  v[pos] = value;
  return v;
}

/**
 * Ensures input data size is a multiple of burst size, by adding additional
 * zero padding if necessary.
 *
 * @returns the number of bytes written, including padding if added
 */
template<typename T>
int64_t writeAndPad(cask::runtime::GeneratedSpmvImplementation* impl,
    int controllerNum,
    int numControllers,
    int64_t startAddress,
    std::vector<T> data,
    std::string routingString)
{
  cutils::align(data, burst_size_bytes);
  int64_t sizeBytes = cutils::size_bytes(data);
  auto sizes = msinglearray(numControllers, controllerNum, sizeBytes);
  auto addrs = msinglearray(numControllers, controllerNum, startAddress);
  impl->write(sizes[controllerNum],
              &sizes[0],
              &addrs[0],
              (uint8_t*)data.data(),
              routingString.c_str());
  return sizeBytes;
}


// write the data for a partition, starting at the given offset
PartitionWriteResult writeDataForPartition(
    cask::runtime::GeneratedSpmvImplementation *impl,
    int offset,
    const Partition& br,
    const std::vector<double>& v,
    int numControllers,
    int controllerNum) {
  // for each partition write this down
  std::string routingString = "split -> tomem" + std::to_string(controllerNum);
  PartitionWriteResult pwr;
  pwr.indptrValuesStartAddress = cutils::align(offset, burst_size_bytes);
  pwr.indptrValuesSize = writeAndPad(impl,
      controllerNum,
      numControllers,
      pwr.indptrValuesStartAddress,
      br.m_indptr_values,
      routingString);

  pwr.vStartAddress = pwr.indptrValuesStartAddress + pwr.indptrValuesSize;
  // XXX, it may not be safe to pad the vector arbitrarily, if the hardware
  // cannot support unpadding it at runtime
  int64_t sizeV = pwr.colptrSize = writeAndPad(impl,
      controllerNum,
      numControllers,
      pwr.vStartAddress,
      v,
      routingString);

  pwr.colptrStartAddress = pwr.vStartAddress + sizeV;
  pwr.colptrSize = writeAndPad(impl,
      controllerNum,
      numControllers,
      pwr.colptrStartAddress,
      br.m_colptr,
      routingString);

  pwr.outStartAddr = pwr.colptrStartAddress + pwr.colptrSize;
  pwr.outSize = br.outSize;
  return pwr;
}

cask::Vector ssarch::spmv(const cask::Vector& x)
{
  using namespace std;

  if (this->impl.dram_reduction_enabled) {
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
  } else if (impl.max_rows < mat.n) {
      stringstream ss;
      ss << "Matrix is too large! Maximum supported rows: ";
      ss << impl.max_rows;
      ss << " actual rows: " << mat.n;
      throw invalid_argument(ss.str());
  }

  int cacheSize = impl.cache_size;

  vector<double> v = x.data;
  cutils::align(v, sizeof(double) * cacheSize);
  cutils::align(v, burst_size_bytes);

  vector<int> nrows, totalCycles, reductionCycles, paddingCycles, colptrSizes, indptrValuesSizes, outputResultSizes;
  vector<long> outputStartAddresses, colptrStartAddresses;
  vector<long> vStartAddresses, indptrValuesStartAddresses;

  int offset = 0;
  int i = 0;

  assert(partitions.size() == impl.num_pipes && "numPipes should equal numPartitions");
  assert(impl.num_pipes % impl.num_controllers == 0 && "numPipes should be a multiple of numControllers");
  assert(impl.num_controllers <= impl.num_pipes && "numPipes should be larger than numControllers");
  for (auto& p : partitions) {
    nrows.push_back(p.n);
    paddingCycles.push_back(p.paddingCycles);
    totalCycles.push_back(p.totalCycles);
    reductionCycles.push_back(p.reductionCycles);

    int nc = impl.num_controllers;
    int pipesPerController = impl.num_pipes / nc;
    int ctrlId = i / pipesPerController;
    // moving to a new controller, reset offset in memory
    if (i % pipesPerController == 0) {
      offset = 0;
    }
    PartitionWriteResult pr = writeDataForPartition(&this->impl, offset, p, v, nc, ctrlId);
    outputStartAddresses.push_back(pr.outStartAddr);
    outputResultSizes.push_back(pr.outSize);
    colptrStartAddresses.push_back(pr.colptrStartAddress);
    colptrSizes.push_back(cutils::size_bytes(p.m_colptr));
    vStartAddresses.push_back(pr.vStartAddress);
    indptrValuesSizes.push_back(cutils::size_bytes(p.m_indptr_values));
    indptrValuesStartAddresses.push_back(pr.indptrValuesStartAddress);

    offset = pr.outStartAddr + p.outSize;
    i++;
  }

  // npartitions and vector load cycles should be the same for all partitions
  int nBlocks = this->partitions[0].nBlocks;
  int vector_load_cycles = this->partitions[0].vector_load_cycles;
  cout << "Running on DFE" << endl;

  int nIterations = 2;
  utils::logResult("Total cycles", totalCycles);
  utils::logResult("Padding cycles", paddingCycles);
  utils::logResult("Reduction cycles", reductionCycles);

  auto start = chrono::_V2::system_clock::now();
  impl.Spmv(
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

  double bwidthEst = impl.num_pipes * impl.input_width * getFrequency() / (1024.0  *
      1024 * 1024) * (8 + 4);
  utils::logResult("Input width ", impl.input_width);
  utils::logResult("Pipes ", impl.num_pipes);

  utils::logResult("Iterations", nIterations);
  utils::logResult("Took (ms)", took);
  utils::logResult("Est (ms)", est);
  utils::logResult("Gflops (est)", gflopsEst);
  utils::logResult("Gflops (actual)", gflopsActual);
  utils::logResult("BWidth (est)", bwidthEst);

  vector<double> total;
  for (size_t i = 0; i < outputStartAddresses.size(); i++) {
    vector<double> tmp(outputResultSizes[i] / sizeof(double), 0);
    int ctrlId = i / (impl.num_pipes / impl.num_controllers);
    auto sizes = msinglearray(impl.num_controllers, ctrlId, utils::size_bytes(tmp));
    auto addrs = msinglearray(impl.num_controllers, ctrlId, outputStartAddresses[i]);
    string routing = "frommem" + std::to_string(ctrlId) + " -> join";
    impl.read(
        sizes[ctrlId],
        &sizes[0],
        &addrs[0],
        (uint8_t*)&tmp[0],
        routing.c_str()
        );
    copy(tmp.begin(), tmp.begin() + nrows[i], back_inserter(total));
  }

  // remove the elements which were only for padding
  if (mat.n < impl.num_pipes) {
    // handles the case where n < num_pipes; in this case extra work is added
    // to prevent a design pipeline stall; here we must remove these
    // unnecessary fller elements from the output
    total.resize(mat.n);
  }
  return Vector{total};
}
void ssarch::preprocess(
    const CsrMatrix& mat) {
  this->mat = mat;

  partitions.clear();
  int rowsPerPartition = mat.n / impl.num_pipes;
  int start = 0;

  if (rowsPerPartition == 0) {
    // handles the relatively uninteresting case where there are fewer rows
    // than pipes; this  arises in several tiny tests, but is unlikely in
    // practice, where there should be more rows than pipes; NB that we need to
    // assign some workload to the pipes, leaving them empty stalls the design;
    Partition p = do_blocking(mat.sliceRows(start, mat.n), impl.cache_size, impl.input_width);
    this->partitions.assign(impl.num_pipes, p);
    for (auto&& p : this->partitions) {
      for (auto&& t : p.m_indptr_values) {
        t.value = 0;
      }
    }
    this->partitions[0] = p;
    return;
  }

  for (int i = 0; i < impl.num_pipes - 1; i++) {
    this->partitions.push_back(
        do_blocking(
          mat.sliceRows(start, rowsPerPartition),
          impl.cache_size, impl.input_width));
    start += rowsPerPartition;
  }

  // put all rows left in the last partition
  this->partitions.push_back(
      do_blocking(mat.sliceRows(start, mat.n - start),
        impl.cache_size, impl.input_width));
}
