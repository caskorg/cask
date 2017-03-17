#ifndef SPMV_H
#define SPMV_H

#include "SparseMatrix.hpp"
#include "Model.hpp"
#include "GeneratedImplSupport.hpp"
#include "Utils.hpp"
#include <boost/property_tree/ptree.hpp>

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


      template<typename U>
        void logResult(std::string s, std::vector<U> vals) {
          logResultR(s);
          for (const auto& v : vals)
            std::cout << v << ",";
          std::cout << std::endl;
        }
      template<typename Arg, typename... Args>
        void logResult(std::string s, Arg a, Args... as) {
          logResultR(s, as...);
          std::cout << a << ",";
          std::cout << std::endl;
        }
      template<typename Arg, typename... Args>
        void logResultR(std::string s, Arg a, Args... as) {
          logResultR(s, as...);
          std::cout << a << ",";
        }
      void logResultR(std::string s) {
          std::cout << "Result " << get_name() << " ";
          std::cout << s << "=";
        }
     public:
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
        return
          impl.params.cache_size == other.impl.params.cache_size &&
          impl.params.input_width == other.impl.params.input_width &&
          impl.params.num_pipes == other.impl.params.num_pipes &&
          impl.params.num_controllers == other.impl.params.num_controllers &&
          impl.params.max_rows == other.impl.params.max_rows;
      }

      boost::property_tree::ptree write_est_impl_params(const model::DeviceModel &dm) {
        auto params = getImplementationParameters(dm);
        boost::property_tree::ptree tree;
        tree.put("memory_bandwidth", params.memoryBandwidth);
        tree.put("BRAMs", params.ru.brams);
        tree.put("LUTs", params.ru.luts);
        tree.put("FFs", params.ru.ffs);
        tree.put("DSPs", params.ru.dsps);
        return tree;
      }
      virtual boost::property_tree::ptree write_params() {
        boost::property_tree::ptree tree;
        tree.put("num_pipes", impl.params.num_pipes);
        tree.put("cache_size", impl.params.cache_size);
        tree.put("input_width", impl.params.input_width);
        tree.put("max_rows", impl.params.max_rows);
        tree.put("num_controllers", impl.params.num_controllers);
        return tree;
      }

      virtual std::string getLibraryName() {
        std::stringstream ss;
        ss << "libSpmv_";
        // XXX detect target from environment configuration?
        const std::string target = "sim";
        ss << target << "_";
        ss << "cachesize" << impl.params.cache_size << "_";
        ss << "inputwidth" << impl.params.input_width << "_";
        ss << "numpipes" << impl.params.num_pipes << "_";
        ss << "maxrows" << impl.params.max_rows;
        ss << "numControllers" << impl.params.num_controllers << "_";
        ss << ".so";
        return ss.str();
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
      virtual model::ImplementationParameters getImplementationParameters(const model::DeviceModel& deviceModel) {
        // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
        //int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;
        using namespace model;

        // It's hard to do this polymorphically; this hack should be fine for
        // small device numbers
        if (deviceModel.getId() == "Max4") {
          // TODO this should be architecture parameter
          const int vectorDataWidthInBits = 64;
          const int entriesPerBram = deviceModel.entriesPerBram(vectorDataWidthInBits);
          const int maxRows = utils::ceilDivide(this->mat.n, impl.params.num_pipes);

          LogicResourceUsage bramReductionKernel{10069, 12965, utils::ceilDivide(maxRows, entriesPerBram), 0};
          LogicResourceUsage paddingKernel{363, 543, 0, 0};
          LogicResourceUsage unpaddingKernel{364, 474, 0, 0};
          LogicResourceUsage spmvKernelPerInput{1458, 2031, impl.params.cache_size / entriesPerBram + 14, 4};
          LogicResourceUsage sm{1235, 643, 0, 0};

          LogicResourceUsage spmvPerPipe =
            bramReductionKernel +
            paddingKernel +
            unpaddingKernel * 3 +
            spmvKernelPerInput * impl.params.input_width +
            sm;

          //std::cout << "Spmv Per pipe usage " << spmvPerPipe.to_string() << std::endl;
          //std::cout << "Spmv per kernel usage " << spmvKernelPerInput.to_string() << std::endl;
          //std::cout << "Spmv reduction per pipe" << bramReductionKernel.to_string() << std::endl;

          // NB these assume an 800Mhz memory bandwidth build
          LogicResourceUsage memoryPerPipe{5325, 12184, 108, 0};
          LogicResourceUsage memory{15330, 17606, 56, 0};

          // There are also a number of FIFOs per pipe, their depth and
          // number is generally independent from design parameters
          LogicResourceUsage fifosPerPipe{500, 800, 81, 0};

          LogicResourceUsage designUsage = (spmvPerPipe + memoryPerPipe + fifosPerPipe) * impl.params.num_pipes + memory;

          double memoryBandwidth =(double)impl.params.input_width * impl.params.num_pipes * getFrequency() * 12.0 / 1E9;
          ImplementationParameters ip{designUsage, memoryBandwidth};
          //ip.clockFrequency = getFrequency() / 1E6; // MHz
          return ip;

        }

        throw std::invalid_argument("Unsupported device model " + deviceModel.getId());
      }

      std::string to_string() {
        std::stringstream s;
        s << get_name();
        s << " " << impl.params.cache_size;
        s << " " << impl.params.input_width;
        s << " " << impl.params.num_pipes;
        s << " " << impl.params.num_controllers;
        s << " " << getEstimatedClockCycles();
        s << " " << getEstimatedGFlops();
        return s.str();
      }

      virtual cask::CsrMatrix preprocessBlock(
          const cask::CsrMatrix& in,
          int blockNumber,
          int nBlocks) {
        return in;
      }

      Vector spmv(const Vector& v);

      void preprocess(const CsrMatrix& mat);

      Spmv(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows, int _numControllers)
          : impl(runtime::ImplementationParameters(
                -1,
                _maxRows,
                _numPipes,
                _cacheSize,
                _inputWidth,
                false, // dram_reduction_enabled
                _numControllers),
                 runtime::stubDeviceInterface)
          {}
      /**
               * For execution we build the architecture and give it a pointer to the
               * device implementation.
               */
      Spmv(runtime::GeneratedSpmvImplementation _impl): impl(_impl) { }
      Partition do_blocking(
          const CsrMatrix& mat,
          int blockSize,
          int inputWidth);
      cask::runtime::GeneratedSpmvImplementation impl;
      cask::CsrMatrix mat;
      std::vector<Partition> partitions;
     protected:
      virtual int countComputeCycles(int32_t* v, int size, int inputWidth);
    };

    inline std::ostream& operator<<(std::ostream& s, Spmv& a) {
      s << " Estimated clock cycles = " << a.getEstimatedClockCycles();
      return s;
    }

    // An ArchitectureSpace provides a simple way to iterate over the design space
    // of an architecture, abstracting the details of the architecture's parameters
    // XXX actually implemnt the STL iterator (if it makes sense?)
    class SpmvArchitectureSpace {
      public:
        std::shared_ptr<Spmv> next() {
          return std::shared_ptr<Spmv>(doNext());
        }

        // restart the exploration process
        virtual void restart() = 0;

      protected:
        // returns nullptr if at end
        virtual Spmv* doNext() = 0;
    };

    template<typename T>
    class SimpleSpmvArchitectureSpace : public SpmvArchitectureSpace {
      // NOTE any raw pointers returned through the API of this class
      // are assumed to be wrapped in smart pointers by the base class
      cask::utils::Range cacheSizeR{1024, 4096, 512};
      cask::utils::Range inputWidthR{8, 100, 8};
      cask::utils::Range numPipesR{1, 6, 1};
      cask::utils::Range numControllersR{1, 6, 1};
      int maxRows;

      bool last = false;

      public:

      SimpleSpmvArchitectureSpace(
          cask::utils::Range numPipesRange,
          cask::utils::Range inputWidthRange,
          cask::utils::Range cacheSizeRange,
          cask::utils::Range numControllersRange,
          int _maxRows) {
        cacheSizeR = cacheSizeRange;
        inputWidthR = inputWidthRange;
        numPipesR = numPipesRange;
        maxRows = _maxRows;
        numControllersR = numControllersRange;
      }

      SimpleSpmvArchitectureSpace() {
      }

      protected:
      void restart() override {
        cacheSizeR.restart();
        inputWidthR.restart();
        numPipesR.restart();
        numControllersR.restart();
        last = false;
      }

      T* doNext(){
        if (last)
          return nullptr;

        T* result = new T(cacheSizeR.crt, inputWidthR.crt, numPipesR.crt, maxRows, numControllersR.crt);

        ++numControllersR;
        if (numControllersR.at_start()) {
          ++cacheSizeR;
          if (cacheSizeR.at_start()) {
            ++inputWidthR;
            if (inputWidthR.at_start()) {
              ++numPipesR;
              if (numPipesR.at_start())
                last = true;
            }
          }
        }

        return result;
      }
    };


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
