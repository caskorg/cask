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
    CsrMatrix a = io::readMatrix("test/systems/tinysym.mtx");
    Vector rhs = cask::io::readVector("test/systems/tinysym_b.mtx");
    Vector v(rhs);

    auto spmv = cc.getSpmv(a);
    spmv.preprocess(a);
    Vector exp = io::readVector("test/systems/tinysym_sol.mtx");
    spmv.spmv(v).print("Spmv res = ");
    exp.print("Exp = ");

    // make test fail until we implement it
    ASSERT_TRUE(false);
}
