#ifndef DSE_H
#define DSE_H

#include <memory>
#include <Spark/Spmv.hpp>
#include <Spark/SimpleSpmv.hpp>
#include <Spark/Utils.hpp>
#include <Spark/io.hpp>
#include <chrono>

namespace spark {
  namespace dse {

    // a benchmark to use for the DSE
    class Benchmark {
      public:
        Benchmark() {
        }
    };


    // the parameters and ranges to use for DSE
    class DseParameters {
      public:
        int numPipes;
        DseParameters() {
        }
    };

    inline std::ostream& operator<<(std::ostream& s, DseParameters& d) {
      s << "DseParams(" << std::endl;
      s << "  numPipes = " << d.numPipes << std::endl;
      s << ")" << std::endl;
      return s;
    }

    struct Params {
      bool gflopsOnly;
      Params(bool _gflopsOnly) : gflopsOnly(_gflopsOnly) {}
    };

    class SparkDse {
      public:
        SparkDse() {}
        void runDse();
        // returns the best architecture
        int run (
            std::string path,
            spark::utils::Range numPipesRange,
            spark::utils::Range inputWidthRange,
            spark::utils::Range cacheSizeRange,
            const Params& params);
    };
  }
}

#endif /* end of include guard: DSE_H */
