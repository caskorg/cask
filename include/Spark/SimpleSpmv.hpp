#ifndef SIMPLESPMV_H
#define SIMPLESPMV_H

#include <sstream>

#include <Spark/Spmv.hpp>
#include <Spark/Utils.hpp>
#include <Spark/Model.hpp>
#include <Spark/device/SpmvDeviceInterface.h>
#include <Spark/GeneratedImplSupport.hpp>


namespace spark {
  namespace spmv {

    using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

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
    class SimpleSpmvArchitecture : public SpmvArchitecture {
      // architecture specific properties
      protected:

      // design parameters
      const int cacheSize, inputWidth, numPipes, maxRows;

      EigenSparseMatrix mat;
      std::vector<Partition> partitions;
      spark::runtime::GeneratedSpmvImplementation* impl;

      virtual int countComputeCycles(uint32_t* v, int size, int inputWidth);

      virtual bool equals(const SpmvArchitecture& a) const override {
        const SimpleSpmvArchitecture* other = dynamic_cast<const SimpleSpmvArchitecture*>(&a);
        return other != nullptr && *other == *this;
      }

      public:

        bool operator==(const SimpleSpmvArchitecture& other) const {
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

        SimpleSpmvArchitecture() :
          cacheSize(getSpmv_PartitionSize()),
          inputWidth(getSpmv_InputWidth()),
          numPipes(getSpmv_NumPipes()),
          maxRows(getSpmv_MaxRows()) {}

        SimpleSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
          cacheSize(_cacheSize),
          inputWidth(_inputWidth),
          numPipes(_numPipes),
          maxRows(_maxRows) {}

        /**
         * For execution we build the architecture and give it a pointer to the
         * device implementation.
         */
        SimpleSpmvArchitecture(spark::runtime::GeneratedSpmvImplementation* _impl):
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
        virtual spark::model::ImplementationParameters getImplementationParameters() {
          // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
          //int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;
          using namespace spark::model;

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

        virtual void preprocess(const EigenSparseMatrix& mat) override;

        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) override;

        virtual std::string get_name() override {
          return std::string("Simple");
        }

      private:
        std::vector<EigenSparseMatrix> do_partition(
            const EigenSparseMatrix& mat,
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
      spark::utils::Range cacheSizeR{1024, 4096, 512};
      spark::utils::Range inputWidthR{8, 100, 8};
      spark::utils::Range numPipesR{1, 6, 1};
      int maxRows;

      bool last = false;

      public:

      SimpleSpmvArchitectureSpace(
          spark::utils::Range numPipesRange,
          spark::utils::Range inputWidthRange,
          spark::utils::Range cacheSizeRange,
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


    // FST based architecture, for now we assume it's the same though it probably isn't
    class FstSpmvArchitecture : public SimpleSpmvArchitecture {
      protected:

      virtual int countComputeCycles(uint32_t* v, int size, int inputWidth) override {
        int cycles = 0;
        for (int i = 0; i < size; i++) {
          int toread = v[i] - (i > 0 ? v[i - 1] : 0);
          do {
            toread -= std::min(toread, inputWidth);
            cycles++;
          } while (toread > 0);
        }
        return cycles;
      }

      public:
      FstSpmvArchitecture() : SimpleSpmvArchitecture() {}

      FstSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
        SimpleSpmvArchitecture(_cacheSize, _inputWidth, _numPipes, _maxRows) {}

      virtual std::string get_name() override {
        return std::string("Fst");
      }

    };

    // Model for an architecture which can skip sequences of empty rows
    class SkipEmptyRowsArchitecture : public SimpleSpmvArchitecture {
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

        virtual spark::sparse::CsrMatrix preprocessBlock(
            const spark::sparse::CsrMatrix& in,
            int blockNumber,
            int nBlocks) override {
          bool encode = blockNumber != 0 && blockNumber != nBlocks - 1;
          return std::make_tuple(
              encodeEmptyRows(std::get<0>(in), encode),
              std::get<1>(in),
              std::get<2>(in));
        }

      public:
      SkipEmptyRowsArchitecture() : SimpleSpmvArchitecture(){}

      SkipEmptyRowsArchitecture(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
        SimpleSpmvArchitecture(_cacheSize, _inputWidth, _numPipes, maxRows) {}

      virtual std::string get_name() override {
        return std::string("SkipEmpty");
      }
    };

    class PrefetchingArchitecture : public SkipEmptyRowsArchitecture {
      protected:

      public:
      PrefetchingArchitecture() : SkipEmptyRowsArchitecture(){}

      PrefetchingArchitecture(int _cacheSize, int  _inputWidth, int _numPipes, int _maxRows) :
        SkipEmptyRowsArchitecture(_cacheSize, _inputWidth, _numPipes, _maxRows) {}

      virtual std::string get_name() override {
        return std::string("PrefetchingArchitecture");
      }
    };

  }
}


#endif /* end of include guard: SIMPLESPMV_H */
