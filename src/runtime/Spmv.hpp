#ifndef SPMV_H
#define SPMV_H

#include "SparseMatrix.hpp"
#include "Model.hpp"
#include "GeneratedImplSupport.hpp"
#include "Utils.hpp"

namespace cask {
  namespace spmv {

  // pack values and colptr to reduce number of streams
#pragma pack(1)
struct indptr_value {
  double value;
  int indptr;
  indptr_value(double _value, int _indptr) : value(_value), indptr(_indptr) {}
  indptr_value() : value(0), indptr(0) {}
} __attribute__((packed));
#pragma pack()

/* A partition is a horizontal stripe of an input matrix. Each
 partition can be further divided into vertical stripes, which
 we call blocks. */
struct Partition {
  int nBlocks, n, paddingCycles, totalCycles, vector_load_cycles, outSize;
  int reductionCycles, emptyCycles;
  int m_colptr_unpaddedLength;
  int m_indptr_values_unpaddedLength;
  std::vector<int> m_colptr;
  std::vector<indptr_value> m_indptr_values;

  std::string to_string() {
    std::stringstream s;
    s << "Vector load cycles " << vector_load_cycles << std::endl;
    s << "Padding cycles = " << paddingCycles << std::endl;
    s << "Total cycles = " << totalCycles << std::endl;
    s << "Nrows = " << n << std::endl;
    s << "Partitions = " << nBlocks << std::endl;
    s << "Reduction cycles = " << reductionCycles << std::endl;
    std::cout << "Empty cycles = " << emptyCycles << std::endl;
    return s.str();
  }
};

    /**
     * Interface for all SpMV implementations. Provides both runtime and design time functions.
     */
    class Spmv {
      CsrMatrix mat;
      std::vector<Partition> partitions;

     public:
      runtime::GeneratedSpmvImplementation impl;
      /** Constructor interface for mock Spmv Implementation to be used during design space exploration */
      Spmv(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows, int _numControllers)
          : impl(-1,
              cask::runtime::spmvRunMock,
              cask::runtime::spmvWriteMock,
              cask::runtime::spmvReadMock,
              _maxRows,
              _numPipes,
              _cacheSize,
              _inputWidth,
              false, // dram_reduction_enabled
              _numControllers) {}
      /**
       * For execution we build the architecture and give it a pointer to the
       * device implementation.
       */
      Spmv(runtime::GeneratedSpmvImplementation _impl): impl(_impl) { }

     public:

      Partition do_blocking(
          const CsrMatrix& mat,
          int blockSize,
          int inputWidth);

      double getEstimatedGFlops() {
        return  getGFlopsCount() * getFrequency() / getEstimatedClockCycles();
      }

      double getFrequency() {
        return 100 * 1E6;
      }

      virtual std::string get_name() {
        return std::string("Simple");
      }

      bool operator==(const Spmv& other) const {
        return impl == other.impl;
      }

      virtual double getEstimatedClockCycles() {
        auto res = max_element(partitions.begin(), partitions.end(),
            [](const Partition& a, const Partition& b) {
              return a.totalCycles < b.totalCycles;
            });
        return res->totalCycles;
      }

      virtual double getGFlopsCount() {
        return 2 * this->mat.nnzs / 1E9;
      }

      /* Returns the expected values for implementing this design on the given deviceModel. */
      model::HardwareModel getEstimatedHardwareModel(const model::DeviceModel& deviceModel) {
        // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
        //int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;
        using namespace model;

        // It's hard to do this polymorphically; this hack should be fine for
        // small device numbers
        if (deviceModel.getId() != "Max4") {
          throw std::invalid_argument("Unsupported device model " + deviceModel.getId());
        }

        // TODO this should be architecture parameter
        const int vectorDataWidthInBits = 64;
        const int entriesPerBram = deviceModel.entriesPerBram(vectorDataWidthInBits);
        const int maxRows = utils::ceilDivide(this->mat.n, impl.num_pipes);

        LogicResourceUsage bramReductionKernel{10069, 12965, utils::ceilDivide(maxRows, entriesPerBram), 0};
        LogicResourceUsage paddingKernel{363, 543, 0, 0};
        LogicResourceUsage unpaddingKernel{364, 474, 0, 0};
        LogicResourceUsage spmvKernelPerInput{1458, 2031, impl.cache_size / entriesPerBram + 14, 4};
        LogicResourceUsage sm{1235, 643, 0, 0};

        LogicResourceUsage spmvPerPipe =
            bramReductionKernel +
                paddingKernel +
                unpaddingKernel * 3 +
                spmvKernelPerInput * impl.input_width +
                sm;

        // NB these assume an 800Mhz memory bandwidth build
        LogicResourceUsage memoryPerPipe{5325, 12184, 108, 0};
        LogicResourceUsage memory{15330, 17606, 56, 0};

        // There are also a number of FIFOs per pipe, their depth and
        // number is generally independent from design parameters
        LogicResourceUsage fifosPerPipe{500, 800, 81, 0};

        LogicResourceUsage designUsage = (spmvPerPipe + memoryPerPipe + fifosPerPipe) * impl.num_pipes + memory;

        double memoryBandwidth =(double)impl.input_width * impl.num_pipes * getFrequency() * 12.0 / 1E9;
        HardwareModel ip{designUsage, memoryBandwidth};
        //ip.clockFrequency = getFrequency() / 1E6; // MHz
        return ip;
      }

      std::string to_string() {
        std::stringstream s;
        s << get_name();
        s << " " << impl.cache_size;
        s << " " << impl.input_width;
        s << " " << impl.num_pipes;
        s << " " << impl.num_controllers;
        s << " " << getEstimatedClockCycles();
        s << " " << getEstimatedGFlops();
        return s.str();
      }

      Vector spmv(const Vector& v);

      void preprocess(const CsrMatrix& mat);

     protected:
      virtual int countComputeCycles(int32_t* v, int size, int inputWidth);

      virtual CsrMatrix preprocessBlock(
          const CsrMatrix& in,
          int blockNumber,
          int nBlocks) {
        return in;
      }

    };

    inline std::ostream& operator<<(std::ostream& s, Spmv& a) {
      s << " Estimated clock cycles = " << a.getEstimatedClockCycles();
      return s;
    }


    // Model for an architecture which can skip sequences of empty rows
    class SkipEmptyRowsSpmv : public Spmv {
      protected:
        std::vector<int32_t> encodeEmptyRows(std::vector<int32_t> pin, bool encode) {
          std::vector<int32_t> encoded;
          if (!encode) {
            return pin;
          }

          int emptyRunLength = 0;
          for (size_t i = 0; i < pin.size(); i++) {
            uint32_t rowLength = pin[i] - (i == 0 ? 0 : pin[i - 1]);
            if (rowLength == 0) {
              emptyRunLength++;
            } else {
              if (emptyRunLength != 0) {
                encoded.push_back(emptyRunLength | (1 << 31));
              }
              emptyRunLength = 0;
              encoded.push_back(pin[i]);
            }
          }

          if (emptyRunLength != 0) {
            encoded.push_back(emptyRunLength | (1 << 31));
          }

          return encoded;
        }

        virtual cask::CsrMatrix preprocessBlock(
            const cask::CsrMatrix& in,
            int blockNumber,
            int nBlocks) override {
          bool encode = blockNumber != 0 && blockNumber != nBlocks - 1;
          cask::CsrMatrix m;
          m.row_ptr = encodeEmptyRows(in.row_ptr, encode);
          m.values = in.values;
          m.col_ind = in.col_ind;
          return m;
        }

      public:
      SkipEmptyRowsSpmv(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows, int _numControllers) :
        Spmv(_cacheSize, _inputWidth, _numPipes, _maxRows, _numControllers) {}

      virtual std::string get_name() override {
        return std::string("SkipEmpty");
      }
    };

}
}


#endif /* end of include guard: SPMV_H */
