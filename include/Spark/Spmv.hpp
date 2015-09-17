#ifndef SPMV_H
#define SPMV_H

#include <Spark/SparseMatrix.hpp>
#include <Spark/converters.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace spmv {

    class ResourceUsage {
      public:
      int luts, ffs, dsps, brams;
      int mluts, mffs, mdsps, mbrams;
      ResourceUsage(int _luts, int _ffs, int _dsps, int _brams) :
        luts(_luts), ffs(_ffs), dsps(_dsps), brams(_brams) {}
      std::string to_string() {
        std::stringstream s;
        s << "LUTs: " << luts << " FFs: " << ffs << " DSPs: " << dsps << " BRAMs: " << brams;
        return s.str();
      }
    };

    // A generic architecture for SpMV
    class SpmvArchitecture {

      // matrix specific properties, only available after process()
      protected:
      double gflopsCount = -1;
      int totalCycles = -1;

      public:
        //doSpmv(iterations, x);

        double getEstimatedGFlops() {
          return  getGFlopsCount() /(getEstimatedClockCycles() / getFrequency());
        }

        double getFrequency() {
          return 100 * 1E6;
        }

        virtual double getEstimatedClockCycles() = 0;
        virtual double getGFlopsCount() = 0;
        virtual std::string to_string() = 0;
        virtual ResourceUsage getResourceUsage() = 0;
        virtual void preprocess(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) = 0;
        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) = 0;
        virtual std::string get_name() = 0;
    };

    std::ostream& operator<<(std::ostream& s, SpmvArchitecture& a) {
      s << " Estimated clock cycles = " << a.getEstimatedClockCycles();
      return s;
    }

    // An ArchitectureSpace provides a simple way to iterate over the design space
    // of an architecture, abstracting the details of the architecture's parameters
    // XXX actually implemnt the STL iterator (if it makes sense?)
    class SpmvArchitectureSpace {
      public:
        std::shared_ptr<SpmvArchitecture> next() {
          return std::shared_ptr<SpmvArchitecture>(doNext());
        }

        // restart the exploration process
        virtual void restart() = 0;

      protected:
        // returns nullptr if at end
        virtual SpmvArchitecture* doNext() = 0;
    };

    // return the properties for the currently linked hardware design
    int getPartitionSize();
    int getNumPipes();
    int getInputWidth();

    template<typename T>
      void align(std::vector<T>& v, int widthInBytes) {
        int limit = widthInBytes / sizeof(T);
        while ((v.size() * sizeof(T)) % widthInBytes != 0 && limit != 0) {
          v.push_back(T{});
          limit--;
        }
      }

}
}


#endif /* end of include guard: SPMV_H */
