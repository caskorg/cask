#include "Spmv.hpp"
#include "Converters.hpp"
#include <iostream>
#include <tuple>
#include <dfesnippets/VectorUtils.hpp>
#include <dfesnippets/Timing.hpp>

#include "GeneratedImplSupport.hpp"
#include "Utils.hpp"

// XXX num controllers is hardcoded
const int numControllers = 2;

using namespace cask::spmv;
using ssarch = cask::spmv::BasicSpmv;
namespace cutils = cask::utils;

// how many cycles does it take to resolve the accesses
int ssarch::countComputeCycles(uint32_t* v, int size, int inputWidth)
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
cask::spmv::Partition ssarch::do_blocking(
    const cask::CsrMatrix& m,
    int blockSize,
    int inputWidth)
{

  const int* indptr = m.col_ind.data();
  const double* values = m.values.data();
  const int* colptr = m.row_ptr.data();
  int rows = m.n;
  int cols = m.m;
  int n = rows;
  //std::cout << "Mat rows " << n << std::endl;

  int nBlocks = cols / blockSize + (cols % blockSize == 0 ? 0 : 1);
  //std::cout << "Npartitions: " << nPartitions << std::endl;

  std::vector<cask::sparse::CsrMatrix> partitions(nBlocks);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < nBlocks; j++) {
      auto& p = std::get<0>(partitions[j]);
      if (p.size() == 0)
        p.push_back(0);
      else
        p.push_back(p.back());
    }
    //std::cout << "i = " << i << std::endl;
    //std::cout << "colptr" << colptr[i] << std::endl;
    for (int j = colptr[i]; j < colptr[i+1]; j++) {
      auto& p = partitions[indptr[j] / blockSize];
      int idxInPartition = indptr[j] - (indptr[j] / blockSize ) * blockSize;
      std::get<1>(p).push_back(idxInPartition);
      std::get<2>(p).push_back(values[j]);
      std::get<0>(p).back()++;
    }
  }

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
    auto& p_colptr = std::get<0>(pp);
    auto& p_indptr = std::get<1>(pp);
    auto& p_values = std::get<2>(pp);

    int diff = std::get<0>(p).size() - p_colptr.size();
    emptyCycles += diff;
    reductionCycles -= diff;
    cycles += this->countComputeCycles(&std::get<0>(p)[0], n, inputWidth) - diff;

    cutils::align(p_indptr, sizeof(int) * inputWidth);
    cutils::align(p_values, sizeof(double) * inputWidth);
    std::copy(p_colptr.begin(), p_colptr.end(), back_inserter(m_colptr));
    for (size_t i = 0; i < p_values.size(); i++)
      m_indptr_value.push_back(indptr_value( p_values[i], p_indptr[i]));
  }

  Partition br;
  br.m_colptr_unpaddedLength = m_colptr.size();
  br.m_indptr_values_unpaddedLength = m_indptr_value.size();
  //std::cout << "m_colptr unaligned size" << m_colptr.size() << std::endl;
  cutils::align(m_colptr, 384);
  cutils::align(m_indptr_value, 384);
  std::vector<double> out(n, 0);
  cutils::align(out, 384);
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
    cask::runtime::GeneratedSpmvImplementation *impl,
    int offset,
    const Partition& br,
    const std::vector<double>& v,
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
  impl->write(sizes[controllerNum],
              &sizes[0],
              &addrs[0],
              (uint8_t*)&br.m_indptr_values[0],
              routingString.c_str());
              // (uint8_t*)&data[0]);

  pwr.vStartAddress = pwr.indptrValuesStartAddress + pwr.indptrValuesSize;
  sizes = msinglearray(numControllers, controllerNum, cutils::size_bytes(v));
  addrs = msinglearray(numControllers, controllerNum, pwr.vStartAddress);
  // std::cout << "Write 2 " << std::endl;
  // cutils::print(sizes, "sizes=");
  // cutils::print(addrs, "addrs=");
  // data  = msinglearray(numControllers, controllerNum, &v[0]);
  impl->write(sizes[controllerNum],
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
  impl->write(sizes[controllerNum],
              &sizes[0],
              &addrs[0],
              (uint8_t*)&br.m_colptr[0],
              routingString.c_str());

  pwr.outStartAddr = pwr.colptrStartAddress + pwr.colptrSize;
  pwr.outSize = br.outSize;
  return pwr;
}


cask::Vector ssarch::spmv(const cask::Vector& x)
{
  using namespace std;

  if (this->impl->getDramReductionEnabled()) {
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
  } else {
    if (maxRows < mat.n) {
      stringstream ss;
      ss << "Matrix is too large! Maximum supported rows: ";
      ss << maxRows;
      ss << " actual rows: " << mat.n;
      throw invalid_argument(ss.str());
    }
  }

  int cacheSize = this->cacheSize;

  vector<double> v = x.data;
  cutils::align(v, sizeof(double) * cacheSize);
  cutils::align(v, 384);

  std::vector<int> nrows, totalCycles, reductionCycles, paddingCycles, colptrSizes, indptrValuesSizes, outputResultSizes;
  std::vector<int> colptrUnpaddedSizes, indptrValuesUnpaddedLengths;
  std::vector<long> outputStartAddresses, colptrStartAddresses;
  std::vector<long> vStartAddresses, indptrValuesStartAddresses;

  int offset = 0;
  int i = 0;
  for (auto& p : this->partitions) {
    nrows.push_back(p.n);
    paddingCycles.push_back(p.paddingCycles);
    totalCycles.push_back(p.totalCycles);
    reductionCycles.push_back(p.reductionCycles);
    colptrUnpaddedSizes.push_back(p.m_colptr_unpaddedLength);
    indptrValuesUnpaddedLengths.push_back(p.m_indptr_values_unpaddedLength);

    PartitionWriteResult pr = writeDataForPartition(this->impl, offset, p, v, i);
    outputStartAddresses.push_back(pr.outStartAddr);
    outputResultSizes.push_back(pr.outSize);
    colptrStartAddresses.push_back(pr.colptrStartAddress);
    colptrSizes.push_back(pr.colptrSize);
    vStartAddresses.push_back(pr.vStartAddress);
    indptrValuesSizes.push_back(pr.indptrValuesSize);
    indptrValuesStartAddresses.push_back(pr.indptrValuesStartAddress);

    // offset = pr.outStartAddr + p.outSize;
    i++;
  }

  // npartitions and vector load cycles should be the same for all partitions
  int nBlocks = this->partitions[0].nBlocks;
  int vector_load_cycles = this->partitions[0].vector_load_cycles;
  std::cout << "Running on DFE" << std::endl;

  int nIterations = 1;
  logResult("Total cycles", totalCycles);
  logResult("Padding cycles", paddingCycles);
  logResult("Reduction cycles", reductionCycles);

  auto start = std::chrono::high_resolution_clock::now();
  this->impl->Spmv(
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
  // TODO need a consistent, single point way to handle params
  // must ensure that params match with build parameters
  double frequency = 100 * 1e6;
  double est =(double) totalCycles[0] / frequency;
  double gflopsEst = (2.0 * (double)this->mat.nnzs / est) / 1E9;
  double gflopsActual = (2.0 * (double)this->mat.nnzs / took) / 1E9;

  double bwidthEst = this->numPipes * this->inputWidth * frequency / (1024.0  *
      1024 * 1024) * (8 + 4);
  logResult("Input width ", this->inputWidth);
  logResult("Pipes ", this->numPipes);

  logResult("Iterations", nIterations);
  logResult("Took (ms)", took);
  logResult("Est (ms)", est);
  logResult("Gflops (est)", gflopsEst);
  logResult("Gflops (actual)", gflopsActual);
  logResult("BWidth (est)", bwidthEst);

  std::vector<double> total;
  std::cout << "Start addresses" << outputStartAddresses.size() << std::endl;
  for (size_t i = 0; i < outputStartAddresses.size(); i++) {
    std::vector<double> tmp(outputResultSizes[i] / sizeof(double), 0);
    auto sizes = msinglearray(numControllers, i, cutils::size_bytes(tmp));
    auto addrs = msinglearray(numControllers, i, outputStartAddresses[i]);
    std::string routing = "frommem" + std::to_string(i) + " -> join";
    this->impl->read(
        sizes[i],
        &sizes[0],
        &addrs[0],
        (uint8_t*)&tmp[0],
        routing.c_str()
        );
    std::copy(tmp.begin(), tmp.begin() + nrows[i], std::back_inserter(total));
  }

  // remove the elements which were only for padding
  return cask::Vector{total};
}

std::vector<cask::CsrMatrix> ssarch::do_partition(const cask::CsrMatrix& mat, int numPipes) {
  std::vector<cask::CsrMatrix> res;
  int rowsPerPartition = mat.n / numPipes;
  int start = 0;
  for (int i = 0; i < numPipes - 1; i++) {
    res.push_back(mat.sliceRows(start, rowsPerPartition));
    start += rowsPerPartition;
  }
  // put all rows left in the last partition
  res.push_back(mat.sliceRows(start, mat.n - start));
  return res;
}

void ssarch::preprocess(
    const cask::CsrMatrix& mat) {
  this->mat = mat;

  int rowsPerPartition = mat.n / numPipes;
  int start = 0;
  for (int i = 0; i < numPipes - 1; i++) {
    this->partitions.push_back(
        do_blocking(
          mat.sliceRows(start, rowsPerPartition),
          this->cacheSize, this->inputWidth));
    start += rowsPerPartition;
  }

  // put all rows left in the last partition
  this->partitions.push_back(
      do_blocking(mat.sliceRows(start, mat.n - start),
        this->cacheSize, this->inputWidth));

}
