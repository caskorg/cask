#ifndef SIMPLESPMV_H
#define SIMPLESPMV_H

#include <sstream>

#include <Spark/Spmv.hpp>

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

    struct BlockingResult {
      int nPartitions, n, paddingCycles, totalCycles, vector_load_cycles, outSize;
      std::vector<int> m_colptr;
      std::vector<indptr_value> m_indptr_values;

      std::string to_string() {
        std::stringstream s;
        s << "Vector load cycles " << vector_load_cycles << std::endl;
        s << "Padding cycles = " << paddingCycles << std::endl;
        s << "Total cycles = " << totalCycles << std::endl;
        s << "Nrows = " << n << std::endl;
        s << "Partitions = " << nPartitions << std::endl;
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
      std::string name;
      std::vector<BlockingResult> partitions;

      virtual int cycleCount(int32_t* v, int size, int inputWidth);

      public:
      // XXX need a way to find these
        SimpleSpmvArchitecture() :
          cacheSize(getPartitionSize()),
          inputWidth(getInputWidth()),
          numPipes(getNumPipes()) {}

        SimpleSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
          cacheSize(_cacheSize),
          inputWidth(_inputWidth),
          numPipes(_numPipes),
          name("SimpleSpmvArchitecture") {}

        SimpleSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes, std::string _name) :
          cacheSize(_cacheSize),
          inputWidth(_inputWidth),
          numPipes(_numPipes),
          name(_name) {}

        // NOTE: only call this after a call to preprocessMatrix
        virtual ResourceUsage getResourceUsage() {
          // XXX bram usage for altera in double precision only (512 deep, 40 bits wide, so need 2 BRAMs)
          int brams = (double)cacheSize * (double)inputWidth / 512.0 * 2.0;
          return ResourceUsage{-1, -1, -1, brams};
        }

        virtual std::string to_string() {
          std::stringstream s;
          s << name;
          s << " cacheSize = " << cacheSize;
          s << " inputWidth = " << inputWidth;
          s << " numPipes = " << numPipes;
          s << " est. cycles = " << getEstimatedClockCycles();
          s << " est. gflops = " << getEstimatedGFlops();
          return s.str();
        }

        virtual void preprocess(const EigenSparseMatrix mat) override;

        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) override;

      private:
        std::vector<EigenSparseMatrix> do_partition(
            const EigenSparseMatrix mat,
            int numPipes);

        BlockingResult do_blocking(
            const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat,
            int blockSize,
            int inputWidth);
    };

    template<typename T>
    class SimpleSpmvArchitectureSpace : public SpmvArchitectureSpace {
      int minCacheSize = 1024;
      int maxCacheSize = 4096 + 512;

      int minInputWidth = 8;
      int maxInputWidth = 100;

      int maxNumPipes = 6;
      int minNumPipes = 1;
      int currentPipes = 1;

      int inputWidth = minInputWidth;
      int cacheSize = minCacheSize;

      T* firstArchitecture, *lastArchitecture, *currentArchitecture;

      public:

      SimpleSpmvArchitectureSpace() {
        firstArchitecture = new T(minCacheSize, minInputWidth, minNumPipes);
        currentArchitecture = firstArchitecture;
        lastArchitecture = new T(maxCacheSize, maxInputWidth, maxNumPipes);
      }

      virtual ~SimpleSpmvArchitectureSpace() {
        free(lastArchitecture);
        free(firstArchitecture);
        if (currentArchitecture != firstArchitecture &&
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);
      }

      T* begin() {
        return firstArchitecture;
      }

      T* end() {
        return lastArchitecture;
      }

      T* operator++(){
        cacheSize = (cacheSize + 512) % maxCacheSize;
        if (cacheSize == 0) {
          cacheSize = minCacheSize;
          inputWidth = std::max((inputWidth + 8) % maxInputWidth, minInputWidth);
          if (inputWidth == minInputWidth)
            currentPipes = std::max((currentPipes + 1) % maxNumPipes, minNumPipes);
        }

        if (currentArchitecture != firstArchitecture &&
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);

        if (cacheSize == minCacheSize &&
            inputWidth == minInputWidth &&
            currentPipes == minNumPipes)
          return lastArchitecture;

        currentArchitecture = new T(cacheSize, inputWidth, currentPipes);
        return currentArchitecture;
      }
    };


    // FST based architecture, for now we assume it's the same though it probably isn't
    class FstSpmvArchitecture : public SimpleSpmvArchitecture {
      protected:

      virtual int cycleCount(int32_t* v, int size, int inputWidth) override {
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
      FstSpmvArchitecture() : SimpleSpmvArchitecture(2048, 48, 1, "FstSpmvArchitecture"){}

      FstSpmvArchitecture(int _cacheSize, int  _inputWidth, int _numPipes) :
        SimpleSpmvArchitecture(_cacheSize, _inputWidth, _numPipes, "FstSpmvArchitecture") {}

    };

    class FstSpmvArchitectureSpace : public SimpleSpmvArchitectureSpace<FstSpmvArchitecture> {
    };
  }
}


#endif /* end of include guard: SIMPLESPMV_H */
