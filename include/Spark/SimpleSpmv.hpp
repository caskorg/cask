#ifndef SIMPLESPMV_H
#define SIMPLESPMV_H

#include <sstream>

#include <Spark/Spmv.hpp>
#include <Spark/Utils.hpp>


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
      int cacheSize, inputWidth, numPipes;

      EigenSparseMatrix mat;
      std::vector<Partition> partitions;

      virtual int countComputeCycles(int32_t* v, int size, int inputWidth);

      public:

        SimpleSpmvArchitecture() :
          cacheSize(getPartitionSize()),
          inputWidth(getInputWidth()),
          numPipes(getNumPipes()) {}

        SimpleSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
          cacheSize(_cacheSize),
          inputWidth(_inputWidth),
          numPipes(_numPipes) {}

        virtual ~SimpleSpmvArchitecture() {}

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
        virtual ImplementationParameters getImplementationParameters() {
          // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
          //int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;

          // XXX these should be architecture params
          int maxRows = 200000;
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

          return ImplementationParameters{designUsage};
        }

        virtual std::string to_string() {
          std::stringstream s;
          s << get_name();
          s << " cacheSize = " << cacheSize;
          s << " inputWidth = " << inputWidth;
          s << " numPipes = " << numPipes;
          s << " est. cycles = " << getEstimatedClockCycles();
          s << " est. gflops = " << getEstimatedGFlops();
          return s.str();
        }

        virtual void preprocess(const EigenSparseMatrix mat) override;

        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) override;

        virtual std::string get_name() override {
          return std::string("SimpleSpmvArchitecture");
        }

      private:
        std::vector<EigenSparseMatrix> do_partition(
            const EigenSparseMatrix mat,
            int numPipes);

        Partition do_blocking(
            const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat,
            int blockSize,
            int inputWidth);
    };

    template<typename T>
    class SimpleSpmvArchitectureSpace : public SpmvArchitectureSpace {
      // NOTE any raw pointers returned through the API of this class
      // are assumed to be wrapped in smart pointers by the base class
      spark::utils::Range cacheSizeR{1024, 4096, 512};
      spark::utils::Range inputWidthR{8, 100, 8};
      spark::utils::Range numPipesR{1, 6, 1};

      bool last = false;

      public:

      SimpleSpmvArchitectureSpace(
          spark::utils::Range numPipesRange,
          spark::utils::Range inputWidthRange,
          spark::utils::Range cacheSizeRange) {
        cacheSizeR = cacheSizeRange;
        inputWidthR = inputWidthRange;
        numPipesR = numPipesRange;
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

        T* result = new T(cacheSizeR.crt, inputWidthR.crt, numPipesR.crt);

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

      virtual int countComputeCycles(int32_t* v, int size, int inputWidth) override {
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
      FstSpmvArchitecture() : SimpleSpmvArchitecture(2048, 48, 1){}

      FstSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
        SimpleSpmvArchitecture(_cacheSize, _inputWidth, _numPipes) {}

      virtual std::string get_name() override {
        return std::string("FstSpmvArchitecture");
      }

    };

    // Model for an architecture which can skip sequences of empty rows
    class SkipEmptyRowsArchitecture : public SimpleSpmvArchitecture {
      protected:

      virtual int countComputeCycles(int32_t* v, int size, int inputWidth) override {
        int cycles = 0;
        int crtPos = 0;
        int prevtoread = -1;
        for (int i = 0; i < size; i++) {
          int toread = v[i] - (i > 0 ? v[i - 1] : 0);
          //only one cycle for empty row
          if (toread == 0 && prevtoread == 0)
            continue;
          if (toread == 0)
            cycles++; // we need to cycles to deal with each empty row
          prevtoread = toread;
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

      public:
      SkipEmptyRowsArchitecture() : SimpleSpmvArchitecture(2048, 48, 1){}

      SkipEmptyRowsArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
        SimpleSpmvArchitecture(_cacheSize, _inputWidth, _numPipes) {}

      virtual std::string get_name() override {
        return std::string("SkipEmptyRowsSpmvArchitecture");
      }
    };

    class PrefetchingArchitecture : public SkipEmptyRowsArchitecture {
      protected:

      public:
      PrefetchingArchitecture() : SkipEmptyRowsArchitecture(2048, 48, 1){}

      PrefetchingArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
        SkipEmptyRowsArchitecture(_cacheSize, _inputWidth, _numPipes) {}

      virtual std::string get_name() override {
        return std::string("PrefetchingArchitecture");
      }
    };

  }
}


#endif /* end of include guard: SIMPLESPMV_H */
