#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SpamSparseMatrix.hpp>
#include <SpamUtils.hpp>
#include <gtest/gtest.h>

class TestLinearSolvers : public ::testing::Test { };

TEST_F(TestLinearSolvers, CGWithIdentityPC) {
   std::vector<double> rhs = spam::io::mm::readVector("tests/systems/tiny_b.mtx");
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("tests/systems/tiny.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   spam::pcg<double, spam::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{1, 2, 3, 4};

   spam::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   spam::print(exp_sol, "exp x = ");
   spam::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
     ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, CGSymWithIdentityPC) {
   std::vector<double> rhs = spam::io::mm::readVector("tests/systems/tinysym_b.mtx");
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("tests/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   spam::pcg<double, spam::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{-2, 2, 3, 3};

   spam::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   spam::print(exp_sol, "exp x = ");
   spam::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
      ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, CGSymWithILUPC) {
   std::vector<double> rhs = spam::io::mm::readVector("tests/systems/tinysym_b.mtx");
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("tests/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   spam::pcg<double, spam::ILUPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{
       -1.9982580059252246,
       2.0000862488691915,
       3.0001293733037859,
       2.9987581910958183
       };

   spam::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   spam::print(exp_sol, "exp x = ");
   spam::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
      ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, ILUCompute2) {
   spam::CsrMatrix a{spam::DokMatrix{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   }};

   spam::ILUPreconditioner ilupc{a};

   spam::DokMatrix exp {
       2,     1,   1,   1,
       0.5, 0.5,   0,   0,
       0.5,   0, 0.5,   0,
       0.5,   0,   0, 0.5
   };

   ASSERT_EQ(ilupc.pc.n, exp.n);
   ASSERT_EQ(ilupc.pc.nnzs, exp.nnzs);
   ASSERT_EQ(ilupc.pc, exp);
}

TEST_F(TestLinearSolvers, ILUCompute) {
   spam::SymCsrMatrix a = spam::io::mm::readSymMatrix("tests/systems/tinysym.mtx");

   spam::CsrMatrix explicitA(a.matrix.toDok().explicitSymmetric());
   std::cout << "--- A (explicit sym) --- " << std::endl;
   explicitA.pretty_print();
   std::cout << "--- A (explicit sym) --- " << std::endl;

   spam::ILUPreconditioner explicitPc{explicitA};
   std::cout << "--- Explicit ILU pc matrix" << std::endl;
   explicitPc.pretty_print();
   std::cout << "--- Explicit ILU pc matrix" << std::endl;

   spam::CsrMatrix csrPc{explicitPc.pc};

   std::vector<int> rows{0, 2, 3, 4, 6};
   std::vector<int> cols{0, 3, 1, 2, 0, 3};
   std::vector<double> vals{1, 1, 1, 1, 1, 1};
   ASSERT_EQ(csrPc.row_ptr, rows);
   ASSERT_EQ(csrPc.col_ind, cols);
   ASSERT_EQ(csrPc.values, vals);
}

TEST_F(TestLinearSolvers, ILUComputeAndApply) {
   spam::ILUPreconditioner ilupc{spam::CsrMatrix{
       spam::DokMatrix{
           2, 1, 1, 1,
           1, 1, 0, 0,
           1, 0, 1, 0,
           1, 0, 0, 1
       }
   }};

   std::vector<double> v{1, 2, 3, 4};
   auto res = ilupc.apply(v);
   std::cout << "Preconditioner " << std::endl;
   ilupc.pretty_print();
   spam::print(v,   "v            = ");
   spam::print(res, "ILU.apply(v) = ");
   std::vector<double> expPcApply{-16.25, 7, 11, 15};
   spam::print(expPcApply, "exp          = ");

   for (auto i = 0u; i < res.size(); i++) {
      ASSERT_DOUBLE_EQ(res[i], expPcApply[i]);
   }
}
