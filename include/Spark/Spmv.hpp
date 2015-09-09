#ifndef SPMV_H
#define SPMV_H

#include <Spark/SparseMatrix.hpp>
#include <Spark/converters.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace spmv {

    Eigen::VectorXd dfespmv(
        Eigen::SparseMatrix<double, Eigen::RowMajor> mat,
        Eigen::VectorXd x);

    Eigen::VectorXd dfespmv(
        const spark::sparse::PartitionedCsrMatrix& result,
        const Eigen::VectorXd& x);

    int getPartitionSize();
    int getInputWidth();

    template<typename T>
      void align(std::vector<T>& v, int widthInBytes) {
        int limit = widthInBytes / sizeof(T);
        while ((v.size() * sizeof(T)) % widthInBytes != 0 && limit != 0) {
          v.push_back(0);
          limit--;
        }
      }


    // how many cycles does it take to resolve the accesses
    int cycleCount(int32_t* v, int size, int inputWidth) {
      int cycles = 0;
      int crtPos = 0;
      int bufferWidth = inputWidth;
      for (int i = 0; i < size; i++) {
        int toread = v[i] - (i > 0 ? v[i - 1] : 0);
        do {
          int canread = std::min(bufferWidth - crtPos, toread);
          crtPos += canread;
          crtPos %= bufferWidth;
          cycles++;
          toread -= canread;
        } while (toread > 0);
      }
      return cycles;
    }

    spark::sparse::PartitionedCsrMatrix partition(
        const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat, int partitionSize)
    {

      const int* indptr = mat.innerIndexPtr();
      const double* values = mat.valuePtr();
      const int* colptr = mat.outerIndexPtr();
      int rows = mat.rows();
      int cols = mat.cols();

      int nPartitions = cols / partitionSize + (cols % partitionSize == 0 ? 0 : 1);
      //std::cout << "Npartitions: " << nPartitions << std::endl;

      std::vector<spark::sparse::CsrMatrix> partitions(nPartitions);

      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < nPartitions; j++) {
          auto& p = std::get<0>(partitions[j]);
          if (p.size() == 0)
            p.push_back(0);
          else
            p.push_back(p.back());
        }
        //std::cout << "i = " << i << std::endl;
        //std::cout << "colptr" << colptr[i] << std::endl;
        for (int j = colptr[i]; j < colptr[i+1]; j++) {
          auto& p = partitions[indptr[j] / partitionSize];
          int idxInPartition = indptr[j] - (indptr[j] / partitionSize ) * partitionSize;
          std::get<1>(p).push_back(idxInPartition);
          std::get<2>(p).push_back(values[j]);
          std::get<0>(p).back()++;
        }
      }
      return partitions;
    }

    //class ResourceUsageModel {
      //int luts, ffs, dsps, brams;
      //int mluts, mffs, mdsps, mbrams;
      //std::string to_string() {
        //stringstream s;
        //s << luts << " " << ffs << " " << dsps << " " << brams;
        //return s.str();
      //}
    //};

    class SpmvArchitecture {

      public:
        //void preprocessMatrix();
        //doSpmv(iterations, x);
        //getResourceUsage();

        double getEstimatedGFlops() {
          return  getGFlopsCount() /(getEstimatedClockCycles() / getFrequency());
        }

        virtual double getGFlopsCount() = 0;
        virtual double getEstimatedClockCycles() = 0;
        virtual double getFrequency() = 0;
        virtual void preprocessMatrix(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) = 0;
        virtual std::string to_string() = 0;
    };

    class SimpleSpmvArchitecture : public SpmvArchitecture {
      // architecture specific properties
      int cacheSize, inputWidth;

      // matrix specific properties, only available after process()
      int totalCycles = -1;
      double gflopsCount = -1;

      public:
        SimpleSpmvArchitecture(int _cacheSize,int  _numPipes) :
          cacheSize(_cacheSize), inputWidth(_numPipes) {}

        double getFrequency() override {
          return 100 * 1E6;
        }

        double getGFlopsCount() override {
          return gflopsCount;
        }

        // NOTE: only call this after a call to preprocessMatrix
        double getEstimatedClockCycles() override {
          return totalCycles;
        }

        virtual std::string to_string() {
          std::stringstream s;
          s << "SpmvArchitecture:";
          s << " cacheSize = " << cacheSize;
          s << " inputWidth = " << inputWidth;
          s << " est. cycles = " << getEstimatedClockCycles();
          s << " est. gflops = " << getEstimatedGFlops();
          return s.str();
        }

        virtual void preprocessMatrix(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) {
          int n = mat.cols();
          auto result = spark::spmv::partition(mat, cacheSize);
          std::vector<double> v(n, 0);
          std::vector<double> m_values;
          std::vector<int> m_colptr, m_indptr;

          int cycles = 0;
          for (auto p : result) {
            auto p_colptr = std::get<0>(p);
            auto p_indptr = std::get<1>(p);
            auto p_values = std::get<2>(p);
            cycles += cycleCount(&p_colptr[0], n, inputWidth);
            align(p_indptr, sizeof(int) * inputWidth);
            align(p_values, sizeof(double) * inputWidth);
            std::copy(p_values.begin(), p_values.end(), back_inserter(m_values));
            std::copy(p_indptr.begin(), p_indptr.end(), back_inserter(m_indptr));
            std::copy(p_colptr.begin(), p_colptr.end(), back_inserter(m_colptr));
          }

          int nPartitions = result.size();
          int outSize = n;
          int paddingCycles = n % 2;
          outSize += paddingCycles;

          std::vector<double> out(outSize , 0);

          align(m_colptr, 16);
          align(m_values, 384);
          align(m_indptr, 384);
          align(v, sizeof(double) * getPartitionSize());
          totalCycles = cycles + v.size();

          gflopsCount = 2.0 * (double)mat.nonZeros() / 1E9;
        }
    };

    // XXX actually implemnt the STL iterator (if it makes sense?)
    class SpmvArchitectureSpace {
      public:
        virtual SpmvArchitecture* begin() = 0;
        virtual SpmvArchitecture* end() = 0;
        virtual SpmvArchitecture* operator++() = 0;
    };


    class SimpleSpmvArchitectureSpace : public SpmvArchitectureSpace {
      int minCacheSize = 1024;
      int minInputWidth = 8;
      int maxCacheSize = 4096;
      int maxInputWidth = 100;

      int inputWidth = minInputWidth;
      int cacheSize = minCacheSize;

      SimpleSpmvArchitecture* firstArchitecture, *lastArchitecture, *currentArchitecture;

      public:

      SimpleSpmvArchitectureSpace() {
        firstArchitecture = new SimpleSpmvArchitecture(minCacheSize, minInputWidth);
        currentArchitecture = firstArchitecture;
        lastArchitecture = new SimpleSpmvArchitecture(maxCacheSize, maxInputWidth);

      }

      virtual ~SimpleSpmvArchitectureSpace() {
        free(lastArchitecture);
        free(firstArchitecture);
        if (currentArchitecture != firstArchitecture &&
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);
      }

      SimpleSpmvArchitecture* begin() {
        return firstArchitecture;
      }

      SimpleSpmvArchitecture* end() {
        return lastArchitecture;
      }

      SimpleSpmvArchitecture* operator++(){
        cacheSize = (cacheSize + 512) % maxCacheSize;
        if (cacheSize == 0) {
          cacheSize = minCacheSize;
          inputWidth = std::max((inputWidth + 8) % maxInputWidth, minInputWidth);
        }

        if (currentArchitecture != firstArchitecture &&
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);

        if (cacheSize == minCacheSize &&
            inputWidth == minInputWidth)
          return lastArchitecture;

        currentArchitecture = new SimpleSpmvArchitecture(cacheSize, inputWidth);
        return currentArchitecture;
    }
  };

}
}


#endif /* end of include guard: SPMV_H */
