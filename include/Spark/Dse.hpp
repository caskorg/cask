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
        bool gflopsOnly;
        spark::utils::Range numPipesRange{1, 4, 1};
        spark::utils::Range inputWidthRange{1, 3, 1};
        spark::utils::Range cacheSizeRange{1024, 2048, 1024};
        DseParameters() {
        }
    };

    inline std::ostream& operator<<(std::ostream& s, DseParameters& d) {
      s << "DseParams(" << std::endl;
      //s << "  numPipes = " << d.numPipes << std::endl;
      s << ")" << std::endl;
      return s;
    }

    class SparkDse {
      public:
        SparkDse() {}
        void runDse();
        // returns the best architecture
        int run (
            std::string path,
            const DseParameters& dseParams);
    };
  }
}

#endif /* end of include guard: DSE_H */
