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
    int cycleCount(int32_t* v, int size) {
      int cycles = 0;
      int crtPos = 0;
      int bufferWidth = getInputWidth();
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
        const Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t> mat)
    {

      int partitionSize = getPartitionSize();
      const int* indptr = mat.innerIndexPtr();
      const double* values = mat.valuePtr();
      const int* colptr = mat.outerIndexPtr();
      int rows = mat.rows();
      int cols = mat.cols();

      int nPartitions = cols / partitionSize + (cols % partitionSize == 0 ? 0 : 1);
      //std::cout << "Npartitions: " << nPartitions << std::endl;

      std::vector<spark::sparse::CsrMatrix> partitions(nPartitions);
      std::cout << "Partition"  << std::endl;

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
      std::cout << "Done Partition" << std::endl;
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
          return getEstimatedClockCycles() / getFrequency();
        }

        virtual double getEstimatedClockCycles() = 0;
        virtual double getFrequency() = 0;
        virtual void preprocessMatrix(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) = 0;
        virtual std::string to_string() = 0;
    };

    class SimpleSpmvArchitecture : public SpmvArchitecture {
      int cacheSize, numPipes;
      public:
        SimpleSpmvArchitecture(int _cacheSize,int  _numPipes) :
          cacheSize(_cacheSize), numPipes(_numPipes) {}

        double getEstimatedClockCycles() override {
          return 100;
        }

        double getFrequency() override {
          return 100 * 1E6;
        }

        virtual std::string to_string() {
          return "SpmvArchitecture(cacheSize = " + std::to_string(cacheSize) +
            ", numPipes = " + std::to_string(numPipes) + ")";
        }

        virtual void preprocessMatrix(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) {
          //mat.makeCompressed();

          int n = mat.cols();
          auto result = spark::spmv::partition(mat);
          std::vector<double> v(n, 0);
          //std::vector<double> total(v.size(), 0);
          std::vector<double> m_values;
          std::vector<int> m_colptr, m_indptr;

          int cycles = 0;
          for (auto p : result) {
            auto p_colptr = std::get<0>(p);
            auto p_indptr = std::get<1>(p);
            auto p_values = std::get<2>(p);
            cycles += cycleCount(&p_colptr[0], n);
            align(p_indptr, sizeof(int) * getInputWidth());
            align(p_values, sizeof(double) * getInputWidth());
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
          //std::cout << "Running on DFE" << std::endl;
          //int vector_load_cycles = v.size() / nPartitions;
          //std::cout << "Running on DFE" << std::endl;
          //int totalCycles = cycles + vector_load_cycles * nPartitions;
          //std::cout << "Vector load cycles " << vector_load_cycles << std::endl;
          //std::cout << "Padding cycles = " << paddingCycles << std::endl;
          //std::cout << "Total cycles = " << totalCycles << std::endl;
          //std::cout << "Nrows = " << n << std::endl;
          //std::cout << "Compute cycles = " << cycles << std::endl;
          //std::cout << "Partitions = " << nPartitions << std::endl;
          //std::cout << "Expected out size = " << out.size() << std::endl;
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
      int minCacheSize = 128;
      int minNumPipes = 1;
      int maxCacheSize = 1024;
      int maxNumPipes = 6;

      int numPipes = minNumPipes;
      int cacheSize = minCacheSize;

      SimpleSpmvArchitecture* firstArchitecture, *lastArchitecture, *currentArchitecture;

      public:

      SimpleSpmvArchitectureSpace() {
        firstArchitecture = new SimpleSpmvArchitecture(minCacheSize, minNumPipes);
        currentArchitecture = firstArchitecture;
        lastArchitecture = new SimpleSpmvArchitecture(maxCacheSize, maxNumPipes);

      }

      virtual ~SimpleSpmvArchitectureSpace() {
        free(lastArchitecture);
        free(firstArchitecture);
        if (currentArchitecture != firstArchitecture &
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);
      }

      SimpleSpmvArchitecture* begin() {
        return firstArchitecture;
      }

      SimpleSpmvArchitecture* end() {
        std::cout << "end(): Here" << std::endl;
        return lastArchitecture;
      }

      SimpleSpmvArchitecture* operator++(){
        std::cout << "operator++: Here" << std::endl;
        std::cout << cacheSize << " " << numPipes << std::endl;
        cacheSize = (cacheSize + 128) % maxCacheSize;
        if (cacheSize == 0) {
          cacheSize = minCacheSize;
          numPipes = std::max((numPipes + 1) % maxNumPipes, minNumPipes);
        }

        if (currentArchitecture != firstArchitecture &&
            currentArchitecture != lastArchitecture)
          free(currentArchitecture);

        if (cacheSize == minCacheSize &&
            numPipes == minNumPipes)
          return lastArchitecture;

        std::cout << "Making a new architecture" << std::endl;
        std::cout << cacheSize << " " << numPipes << std::endl;
        currentArchitecture = new SimpleSpmvArchitecture(cacheSize, numPipes);
        return currentArchitecture;
    }
  };

}
}


#endif /* end of include guard: SPMV_H */
