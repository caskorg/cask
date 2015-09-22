#ifndef SPMV_H
#define SPMV_H

#include <Spark/SparseMatrix.hpp>
#include <Spark/converters.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace spmv {

    class LogicResourceUsage {
      public:
      int luts, ffs, brams, dsps;

      LogicResourceUsage() : luts(0), ffs(0), dsps(0), brams(0) {}

      LogicResourceUsage(int _l, int _f, int _b, int _d) :
        luts(_l), ffs(_f), brams(_b), dsps(_d) {}

      LogicResourceUsage(const LogicResourceUsage& ru) {
        luts = ru.luts;
        ffs = ru.ffs;
        dsps = ru.dsps;
        brams = ru.brams;
      }

      std::string to_string() {
        std::stringstream s;
        s << "LUTs: " << luts << " FFs: " << ffs << " DSPs: " << dsps << " BRAMs: " << brams;
        return s.str();
      }

      const LogicResourceUsage operator+(const LogicResourceUsage& ru) const {
        return LogicResourceUsage(luts + ru.luts, ffs + ru.ffs, brams + ru.brams, dsps + ru.dsps);
      }

      const LogicResourceUsage operator*(int x) const {
        return LogicResourceUsage(x * luts,  x * ffs, x * brams, x * dsps);
      }

      const bool operator<(const LogicResourceUsage& ru) {
        return
          luts < ru.luts &&
          ffs < ru.ffs &&
          dsps < ru.dsps &&
          brams < ru.brams;
      }
    };

    // models the implementation parameters which are constrained (e.g. by
    // physical properties, such as lut usage, or by the compiler, such as the
    // number of streams)
    class ImplementationParameters {
      public:

        LogicResourceUsage ru;

        //  params belows not being used currently
        int memoryBandwidth; // <-- would be nice to have this
        int streams;
        int clockFrequency;
        int memoryClockFrequency;

      ImplementationParameters(const LogicResourceUsage& _ru) : ru(_ru) {}

      std::string to_string() {
        std::stringstream s;
        s << ru.to_string();
        s << "Clock frequency " << clockFrequency;
        return s.str();
      }

    };

    // A generic architecture for SpMV
    class SpmvArchitecture {

      public:
        double getEstimatedGFlops() {
          return  getGFlopsCount() /(getEstimatedClockCycles() / getFrequency());
        }

        double getFrequency() {
          return 100 * 1E6;
        }

        virtual double getEstimatedClockCycles() = 0;
        virtual double getGFlopsCount() = 0;
        virtual std::string to_string() = 0;
        virtual ImplementationParameters getImplementationParameters() = 0;
        virtual void preprocess(const Eigen::SparseMatrix<double, Eigen::RowMajor> mat) = 0;
        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) = 0;
        virtual std::string get_name() = 0;

      protected:
        virtual spark::sparse::CsrMatrix preprocessBlock(
            const spark::sparse::CsrMatrix& in,
            int blockNumber,
            int nBlocks) {
          return in;
        }
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
