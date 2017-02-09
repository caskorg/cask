#ifndef SPARSEMATRIX_HPP_1BWMMLC8
#define SPARSEMATRIX_HPP_1BWMMLC8

#include <tuple>
#include <vector>

namespace cask {
  namespace sparse {

    using CsrMatrix = std::tuple<std::vector<uint32_t>, std::vector<int>, std::vector<double>>;

    using PartitionedCsrMatrix = std::vector<CsrMatrix>;

    template<typename value_type>
      class SparkCooMatrix {

        public:
        using CoordType = std::tuple<int, int, value_type>;

        int n, m;
        std::vector<CoordType> data;

        SparkCooMatrix(int _n, int _m) : n(_n), m(_m) {}
      };
  }
}

#endif /* end of include guard: SPARSEMATRIX_HPP_1BWMMLC8 */
