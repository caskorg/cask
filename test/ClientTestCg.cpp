#include <gtest/gtest.h>
#include <Cask.hpp>
#include <IO.hpp>

using namespace std;
using namespace cask;

TEST(ClientCg, SimpleSystem) {
  // Attempt one
  CaskContext cc;
  SymCsrMatrix a = cask::io::readSymMatrix("test/systems/tiny.mtx");
  Vector rhs = cask::io::readVector("test/systems/tiny_b.mtx");
  cask::Vector v(rhs);

  solvers::Cg cg = cc.getCg(a);
  cg.preprocess(a);
  Vector res = cg.solve(v);

  // make test fail until we implement it
  ASSERT_TRUE(false);
}
