#include <Spark/BiConjugateGradient.hpp>

int main() {
  spark::bicg::DfeBiCg bicg{};
  bicg.solve();
  return 0;
}
