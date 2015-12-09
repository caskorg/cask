#ifndef SPMV_H
#define SPMV_H

#include <Spark/SparseMatrix.hpp>
#include <Spark/Model.hpp>
#include <boost/property_tree/ptree.hpp>
#include <Eigen/Sparse>

namespace spark {
  namespace spmv {

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
        virtual spark::model::ImplementationParameters getImplementationParameters() = 0;
        virtual void preprocess(const Eigen::SparseMatrix<double, Eigen::RowMajor>& mat) = 0;
        virtual Eigen::VectorXd dfespmv(Eigen::VectorXd x) = 0;
        virtual std::string get_name() = 0;
        virtual boost::property_tree::ptree write_params() = 0;
        virtual boost::property_tree::ptree write_est_impl_params() = 0;
        virtual std::string getLibraryName() = 0;
        bool operator==(const SpmvArchitecture& other) {
          return equals(other);
        }

        virtual ~SpmvArchitecture() {}

      protected:
        virtual spark::sparse::CsrMatrix preprocessBlock(
            const spark::sparse::CsrMatrix& in,
            int blockNumber,
            int nBlocks) {
          return in;
        }
        virtual bool equals(const SpmvArchitecture& a) const = 0;
    };

    inline std::ostream& operator<<(std::ostream& s, SpmvArchitecture& a) {
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
