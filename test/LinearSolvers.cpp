#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <gtest/gtest.h>

class TestLinearSolvers : public ::testing::Test { };

TEST_F(TestLinearSolvers, CGWithIdentityPC) {
   std::vector<double> rhs = spam::io::readVector("test/systems/tiny_b.mtx");
   spam::CsrMatrix a = spam::io::readMatrix("test/systems/tiny.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   bool converged = spam::pcg<double, spam::IdentityPreconditioner>(a, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{1, 2, 3, 4};
   ASSERT_EQ(sol, exp_sol);
}

TEST_F(TestLinearSolvers, CGSymWithIdentityPC) {
   std::vector<double> rhs = spam::io::readVector("test/systems/tinysym_b.mtx");
   spam::CsrMatrix a = spam::io::readMatrix("test/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   bool converged = spam::pcg<double, spam::IdentityPreconditioner>(a, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{-2, 2, 3, 3};
   EXPECT_EQ(sol, exp_sol);
}

TEST_F(TestLinearSolvers, ILU2) {
   spam::CsrMatrix a{spam::DokMatrix{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   }};

   a.pretty_print();

   spam::ILUPreconditioner ilupc{
       spam::CsrMatrix{
       }};

   spam::DokMatrix expPc{
       2, 1, 1, 1,
       0.5, 0,5, 0, 0,
       0.5, 0, 0.5, 0,
       0.5, 0, 0, 0.5
   };

   ASSERT_EQ(ilupc.pc, spam::CsrMatrix{expPc});
}

TEST_F(TestLinearSolvers, ILU) {
   spam::CsrMatrix a = spam::io::readMatrix("test/systems/tinysym.mtx");

   spam::CsrMatrix explicitA(a.toDok().explicitSymmetric());
   std::cout << "--- A (explicit sym) --- " << std::endl;
   explicitA.pretty_print();
   std::cout << "--- A (explicit sym) --- " << std::endl;

   spam::ILUPreconditioner explicitPc{explicitA};
   std::cout << "--- Explicit ILU pc matrix" << std::endl;
   explicitPc.pretty_print();
   std::cout << "--- Explicit ILU pc matrix" << std::endl;

   std::vector<int> rows{1, 3, 4, 5, 7};
   std::vector<int> cols{1, 4, 2, 3, 1, 4};
   std::vector<double> vals{1, 1, 1, 1, 1, 1};
   ASSERT_EQ(explicitPc.pc.row_ptr, rows);
   ASSERT_EQ(explicitPc.pc.col_ind, cols);
   ASSERT_EQ(explicitPc.pc.values, vals);
}
