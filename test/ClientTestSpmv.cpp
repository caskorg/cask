#include <Cask.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <vector>
#include <gtest/gtest.h>

using namespace std;

/** A high level test of the Cask SpMV interface */
TEST(ClientTestSpmv, TinySymSpmv) {
    using namespace cask;

    CaskContext cc;
    SymCsrMatrix a = io::readSymMatrix("test/systems/tinysym.mtx");
    std::vector<double> rhs = cask::io::readVector("test/systems/tinysym_b.mtx");
    Vector v(rhs);

    auto spmv = cc.getSpmv(a);
    // spmv.preprocess(a);
    // Vector res = spmv.spmv(a, v);
    // std::vector<double> exp = io::readVector("test/systems/tinysym_sol.mtx");
    // Vector vExp{exp};
    // ASSERT_EQ(vExp, res);
}
