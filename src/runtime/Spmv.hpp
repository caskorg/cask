#ifndef SPMV_H
#define SPMV_H

#include "SparseMatrix.hpp"
#include "Model.hpp"
#include "GeneratedImplSupport.hpp"
#include "Utils.hpp"
#include <boost/property_tree/ptree.hpp>
#include <Eigen/Sparse>

namespace cask {
  namespace spmv {

  /**
   * Interface for all SpMV implementations. Provides both runtime and design time functions.
   */
    class Spmv {

      public:
        double getEstimatedGFlops() {
          return  getGFlopsCount() * getFrequency() / getEstimatedClockCycles();
        }

        double getFrequency() {
          return 100 * 1E6;
        }

        virtual double getEstimatedClockCycles() = 0;
        virtual double getGFlopsCount() = 0;
        virtual std::string to_string() = 0;
        virtual cask::model::ImplementationParameters getImplementationParameters() = 0;
        virtual void preprocess(const cask::sparse::EigenSparseMatrix& mat) = 0;
        virtual cask::Vector spmv(const cask::Vector& x) = 0;
        virtual std::string get_name() = 0;
        virtual boost::property_tree::ptree write_params() = 0;
        virtual boost::property_tree::ptree write_est_impl_params() = 0;
        virtual std::string getLibraryName() = 0;
        bool operator==(const Spmv& other) {
          return equals(other);
        }

        virtual ~Spmv() {}

      protected:
        virtual cask::sparse::CsrMatrix preprocessBlock(
            const cask::sparse::CsrMatrix& in,
            int blockNumber,
            int nBlocks) {
          return in;
        }
        virtual bool equals(const Spmv& a) const = 0;
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

    // pack values and colptr to reduce number of streams
#pragma pack(1)
    struct indptr_value {
      double value;
      int indptr;
      indptr_value(double _value, int _indptr) : value(_value), indptr(_indptr) {}
      indptr_value() : value(0), indptr(0) {}
    } __attribute__((packed));
#pragma pack()

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

    // A parameterised, generic architecture for SpMV. Supported parameters are:
    // - input width
    // - number of pipes
    // - cache size per pipe
    // - TODO data type
    class BasicSpmv : public Spmv {
      // architecture specific properties
      protected:

      // design parameters
      const int cacheSize, inputWidth, numPipes, maxRows;

      cask::sparse::EigenSparseMatrix mat;
      std::vector<Partition> partitions;
      cask::runtime::GeneratedSpmvImplementation* impl;

      virtual int countComputeCycles(uint32_t* v, int size, int inputWidth);

      virtual bool equals(const Spmv& a) const override {
        const BasicSpmv* other = dynamic_cast<const BasicSpmv*>(&a);
        return other != nullptr && *other == *this;
      }

      public:

        bool operator==(const BasicSpmv& other) const {
          return
            cacheSize == other.cacheSize &&
            inputWidth == other.inputWidth &&
            numPipes == other.numPipes &&
            maxRows == other.maxRows;
        }

        virtual boost::property_tree::ptree write_est_impl_params() override {
          auto params = getImplementationParameters();
          boost::property_tree::ptree tree;
          tree.put("memory_bandwidth", params.memoryBandwidth);
          tree.put("BRAMs", params.ru.brams);
          tree.put("LUTs", params.ru.luts);
          tree.put("FFs", params.ru.ffs);
          tree.put("DSPs", params.ru.dsps);
          return tree;
        }

        virtual boost::property_tree::ptree write_params() override {
          boost::property_tree::ptree tree;
          tree.put("num_pipes", numPipes);
          tree.put("cache_size", cacheSize);
          tree.put("input_width", inputWidth);
          tree.put("max_rows", maxRows);
          return tree;
        }

        BasicSpmv(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
          cacheSize(_cacheSize),
          inputWidth(_inputWidth),
          numPipes(_numPipes),
          maxRows(_maxRows) {}

        /**
         * For execution we build the architecture and give it a pointer to the
         * device implementation.
         */
        BasicSpmv(cask::runtime::GeneratedSpmvImplementation* _impl):
          cacheSize(_impl->cacheSize()),
          inputWidth(_impl->inputWidth()),
          numPipes(_impl->numPipes()),
          maxRows(_impl->maxRows()),
          impl(_impl) { }

        virtual std::string getLibraryName() {
          std::stringstream ss;
          ss << "libSpmv_";
          // XXX detect target from environment configuration?
          const std::string target = "sim";
          ss << target << "_";
          ss << "cachesize" << cacheSize << "_";
          ss << "inputwidth" << inputWidth << "_";
          ss << "numpipes" << numPipes << "_";
          ss << "maxrows" << maxRows;
          ss << ".so";
          return ss.str();
        }

        virtual double getEstimatedClockCycles() {
          auto res = std::max_element(partitions.begin(), partitions.end(),
              [](const Partition& a, const Partition& b) {
                return a.totalCycles < b.totalCycles;
              });
          return res->totalCycles;
        }

        virtual double getGFlopsCount() {
          return 2 * this->mat.nonZeros() / 1E9;
        }

        // NOTE: only call this after a call to preprocessMatrix
        // XXX this should be renamed to estimated
        // XXX This can't really be done at compile time
        virtual cask::model::ImplementationParameters getImplementationParameters() {
          // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
          //int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;
          using namespace cask::model;

          // XXX these should be architecture params
          int maxRows = (this->mat.rows() / 512) * 512;
          const int virtex6EntriesPerBram = 512;

          LogicResourceUsage interPartitionReductionKernel(2768,1505, maxRows / virtex6EntriesPerBram, 0);
          LogicResourceUsage paddingKernel{400, 500, 0, 0};
          LogicResourceUsage spmvKernelPerInput{1466, 2060, cacheSize / virtex6EntriesPerBram, 10}; // includes cache
          LogicResourceUsage sm{800, 500, 0, 0};

          LogicResourceUsage spmvPerPipe =
            interPartitionReductionKernel +
            paddingKernel +
            spmvKernelPerInput * inputWidth +
            sm;

          LogicResourceUsage memoryPerPipe{3922, 8393, 160, 0};
          LogicResourceUsage memory{24000, 32000, 0, 0};

          //LogicResourceUsage designOther{};
          LogicResourceUsage designUsage = (spmvPerPipe + memoryPerPipe) * numPipes + memory;

          double memoryBandwidth =(double)inputWidth * numPipes * getFrequency() * 12.0 / 1E9;
          ImplementationParameters ip{designUsage, memoryBandwidth};
          //ip.clockFrequency = getFrequency() / 1E6; // MHz
          return ip;
        }

        virtual std::string to_string() {
          std::stringstream s;
          s << get_name();
          s << " " << cacheSize;
          s << " " << inputWidth;
          s << " " << numPipes;
          s << " " << getEstimatedClockCycles();
          s << " " << getEstimatedGFlops();
          return s.str();
        }

        virtual void preprocess(const cask::sparse::EigenSparseMatrix& mat) override;

        virtual cask::Vector spmv(const cask::Vector& v) override;

        virtual std::string get_name() override {
          return std::string("Simple");
        }

      private:
        std::vector<cask::sparse::EigenSparseMatrix> do_partition(
            const cask::sparse::EigenSparseMatrix& mat,
            int numPipes);

        Partition do_blocking(
            const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>& mat,
            int blockSize,
            int inputWidth);

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


    };

    template<typename T>
    class SimpleSpmvArchitectureSpace : public SpmvArchitectureSpace {
      // NOTE any raw pointers returned through the API of this class
      // are assumed to be wrapped in smart pointers by the base class
      cask::utils::Range cacheSizeR{1024, 4096, 512};
      cask::utils::Range inputWidthR{8, 100, 8};
      cask::utils::Range numPipesR{1, 6, 1};
      int maxRows;

      bool last = false;

      public:

      SimpleSpmvArchitectureSpace(
          cask::utils::Range numPipesRange,
          cask::utils::Range inputWidthRange,
          cask::utils::Range cacheSizeRange,
          int _maxRows) {
        cacheSizeR = cacheSizeRange;
        inputWidthR = inputWidthRange;
        numPipesR = numPipesRange;
        maxRows = _maxRows;
      }

      SimpleSpmvArchitectureSpace() {
      }

      protected:
      void restart() override {
        cacheSizeR.restart();
        inputWidthR.restart();
        numPipesR.restart();
        last = false;
      }

      T* doNext(){
        if (last)
          return nullptr;

        T* result = new T(cacheSizeR.crt, inputWidthR.crt, numPipesR.crt, maxRows);

        ++cacheSizeR;
        if (cacheSizeR.at_start()) {
          ++inputWidthR;
          if (inputWidthR.at_start()) {
            ++numPipesR;
            if (numPipesR.at_start())
              last = true;
          }
        }

        return result;
      }
    };


    // Model for an architecture which can skip sequences of empty rows
    class SkipEmptyRowsSpmv : public BasicSpmv {
      protected:

        std::vector<uint32_t> encodeEmptyRows(std::vector<uint32_t> pin, bool encode) {
          std::vector<uint32_t> encoded;
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

        virtual cask::sparse::CsrMatrix preprocessBlock(
            const cask::sparse::CsrMatrix& in,
            int blockNumber,
            int nBlocks) override {
          bool encode = blockNumber != 0 && blockNumber != nBlocks - 1;
          return std::make_tuple(
              encodeEmptyRows(std::get<0>(in), encode),
              std::get<1>(in),
              std::get<2>(in));
        }

      public:
      SkipEmptyRowsSpmv(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
        BasicSpmv(_cacheSize, _inputWidth, _numPipes, maxRows) {}

      virtual std::string get_name() override {
        return std::string("SkipEmpty");
      }
    };

}
}


#endif /* end of include guard: SPMV_H */
