#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <gtest/gtest.h>

class TestLinearSolvers : public ::testing::Test { };

TEST_F(TestLinearSolvers, CGWithIdentityPC) {
   std::vector<double> rhs = spam::io::mm::readVector("test/systems/tiny_b.mtx");
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("test/systems/tiny.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   spam::pcg<double, spam::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{1, 2, 3, 4};

   spam::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   spam::print(exp_sol, "exp x = ");
   spam::print(sol, "got x = ");

   for (auto i = 0u; i < sol.size(); i++) {
     ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, CGSymWithIdentityPC) {
   std::vector<double> rhs = spam::io::mm::readVector("test/systems/tinysym_b.mtx");
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("test/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   spam::pcg<double, spam::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{-2, 2, 3, 3};

   spam::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   spam::print(exp_sol, "exp x = ");
   spam::print(sol, "got x = ");

   for (auto i = 0u; i < sol.size(); i++) {
      ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, ILU2) {
   spam::CsrMatrix a{spam::DokMatrix{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   }};

   spam::ILUPreconditioner ilupc{a};

   spam::CsrMatrix exp{
       spam::DokMatrix{
           2,     1,   1,   1,
           0.5, 0.5,   0,   0,
           0.5,   0, 0.5,   0,
           0.5,   0,   0, 0.5
       }};

   ASSERT_EQ(ilupc.pc.n, exp.n);
   ASSERT_EQ(ilupc.pc.nnzs, exp.nnzs);
   ASSERT_EQ(ilupc.pc.values, exp.values);
   ASSERT_EQ(ilupc.pc.row_ptr, exp.row_ptr);
   ASSERT_EQ(ilupc.pc.col_ind, exp.col_ind);
}

TEST_F(TestLinearSolvers, ILU) {
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("test/systems/tinysym.mtx");

   spam::CsrMatrix explicitA(a.matrix.toDok().explicitSymmetric());
   std::cout << "--- A (explicit sym) --- " << std::endl;
   explicitA.pretty_print();
   std::cout << "--- A (explicit sym) --- " << std::endl;

   spam::ILUPreconditioner explicitPc{explicitA};
   std::cout << "--- Explicit ILU pc matrix" << std::endl;
   explicitPc.pretty_print();
   std::cout << "--- Explicit ILU pc matrix" << std::endl;

   std::vector<int> rows{0, 2, 3, 4, 6};
   std::vector<int> cols{0, 3, 1, 2, 0, 3};
   std::vector<double> vals{1, 1, 1, 1, 1, 1};
   ASSERT_EQ(explicitPc.pc.row_ptr, rows);
   ASSERT_EQ(explicitPc.pc.col_ind, cols);
   ASSERT_EQ(explicitPc.pc.values, vals);
}
