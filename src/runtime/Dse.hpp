#ifndef DSE_H
#define DSE_H

#include <memory>
#include "Spmv.hpp"
#include "Utils.hpp"
#include "IO.hpp"
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace cask {
  namespace dse {

    // a benchmark to use for the DSE
    class Benchmark {
      std::vector<std::string> paths;
      public:
        std::string get_matrix_path(int id) const {
          if (id < paths.size())
            return paths[id];
          std::stringstream ss;
          ss << "Benchmark::Index out of range " << id;
          throw std::invalid_argument(ss.str());
        }

        void add_matrix_path(std::string path) {
          paths.push_back(path);
        }

        int get_benchmark_size() const {
          return paths.size();
        }
    };

    inline std::ostream& operator<<(std::ostream& s, Benchmark& b) {
      s << "Benchmark(" << std::endl;
      for (int i = 0; i < b.get_benchmark_size(); i++)
        s << "  " << b.get_matrix_path(i) << std::endl;
      s << ")" << std::endl;
      return s;
    }

    // the parameters and ranges to use for DSE
    class DseParameters {
      public:
        bool gflopsOnly;
        cask::utils::Range numPipesRange{1, 4, 1};
        cask::utils::Range inputWidthRange{1, 3, 1};
        cask::utils::Range cacheSizeRange{1024, 2048, 1024};
    };

    inline std::ostream& operator<<(std::ostream& s, DseParameters& d) {
      s << "DseParams(" << std::endl;
      s << "  numPipes   = " << d.numPipesRange << std::endl;
      s << "  inputWidth = " << d.inputWidthRange << std::endl;
      s << "  cacheSize  = " << d.cacheSizeRange << std::endl;
      s << ")" << std::endl;
      return s;
    }

    class DseResult {
      public:
        std::shared_ptr<cask::spmv::Spmv> bestArchitecture;
        std::vector<std::string> matrices;

        DseResult(std::string path, std::shared_ptr<cask::spmv::Spmv> arch) {
          matrices.push_back(path);
          bestArchitecture = arch;
        }
        DseResult(std::shared_ptr<cask::spmv::Spmv> arch) {
          bestArchitecture = arch;
        }
    };

    class SparkDse {
      public:
        SparkDse() {}
        // returns the best architecture
        std::vector<DseResult> run (
            const Benchmark& benchmark,
            const DseParameters& dseParams);
    };
  }
}

#endif /* end of include guard: DSE_H */
