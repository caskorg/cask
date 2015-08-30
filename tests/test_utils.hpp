#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <vector>
#include <tuple>
#include <iostream>

#include <dfesnippets/NumericUtils.hpp>

namespace spark {
  namespace test {

    using MismatchT = std::vector<std::tuple<int, double, double>>;

    MismatchT check(
        const std::vector<double>& got,
        const std::vector<double>& exp
        ) {
      std::vector<std::tuple<int, double, double>> mismatches;
      for (int i = 0; i < got.size(); i++) {
        if (!dfesnippets::numeric_utils::almost_equal(got[i], exp[i], 1E-10, 1E-15))
          mismatches.push_back(std::make_tuple(i, got[i], exp[i]));
      }

      return mismatches;
    }

    void print_mismatches(const MismatchT& mismatches) {
      if (!mismatches.empty()) {
        std::cout << "Results didn't match" << std::endl;
        for (int i = 0; i < mismatches.size(); i++) {
          std::cout << "At " << std::get<0>(mismatches[i]);
          std::cout << " got: " << std::get<1>(mismatches[i]);
          std::cout << " exp: "  << std::get<2>(mismatches[i]) << std::endl;
        }
      }
    }
  }
}


#endif /* end of include guard: TEST_UTILS_H */
