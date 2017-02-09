#include <vector>
#include <Benchmark.hpp>
#include <SparseLinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <gtest/gtest.h>

class TestLinearSolvers : public ::testing::Test { };

TEST_F(TestLinearSolvers, CGWithIdentityPC) {
   std::vector<double> rhs = cask::io::readVector("tests/systems/tiny_b.mtx");
   cask::SymCsrMatrix a = cask::io::readSymMatrix("tests/systems/tiny.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   cask::sparse_linear_solvers::pcg<double, cask::sparse_linear_solvers::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{1, 2, 3, 4};

   cask::utils::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   cask::utils::print(exp_sol, "exp x = ");
   cask::utils::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
     ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, CGSymWithIdentityPC) {
   std::vector<double> rhs = cask::io::readVector("tests/systems/tinysym_b.mtx");
   cask::SymCsrMatrix a = cask::io::readSymMatrix("tests/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   cask::sparse_linear_solvers::pcg<double, cask::sparse_linear_solvers::IdentityPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{-2, 2, 3, 3};

   cask::utils::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   cask::utils::print(exp_sol, "exp x = ");
   cask::utils::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
      ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, CGSymWithILUPC) {
   std::vector<double> rhs = cask::io::readVector("tests/systems/tinysym_b.mtx");
   cask::SymCsrMatrix a = cask::io::readSymMatrix("tests/systems/tinysym.mtx");
   int iterations = 0;
   std::vector<double> sol(a.n);
   cask::sparse_linear_solvers::pcg<double, cask::sparse_linear_solvers::ILUPreconditioner>(a.matrix, &rhs[0], &sol[0], iterations);
   std::vector<double> exp_sol{
       -1.9982580059252246,
       2.0000862488691915,
       3.0001293733037859,
       2.9987581910958183
       };

   cask::utils::print(rhs, "b = ");
   std::cout << "Matrix" << std::endl;
   a.pretty_print();
   cask::utils::print(exp_sol, "exp x = ");
   cask::utils::print(sol, "got x = ");
   std::cout << "Iterations = " << iterations << std::endl;

   for (auto i = 0u; i < sol.size(); i++) {
      ASSERT_DOUBLE_EQ(sol[i], exp_sol[i]);
   }
}

TEST_F(TestLinearSolvers, ILUCompute2) {
   cask::CsrMatrix a{cask::DokMatrix{
       2, 1, 1, 1,
       1, 1, 0, 0,
       1, 0, 1, 0,
       1, 0, 0, 1
   }};

   cask::sparse_linear_solvers::ILUPreconditioner ilupc{a};

   cask::DokMatrix exp {
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
   cask::SymCsrMatrix a = cask::io::readSymMatrix("tests/systems/tinysym.mtx");

   cask::CsrMatrix explicitA(a.matrix.toDok().explicitSymmetric());
   std::cout << "--- A (explicit sym) --- " << std::endl;
   explicitA.pretty_print();
   std::cout << "--- A (explicit sym) --- " << std::endl;

   cask::sparse_linear_solvers::ILUPreconditioner explicitPc{explicitA};
   std::cout << "--- Explicit ILU pc matrix" << std::endl;
   explicitPc.pretty_print();
   std::cout << "--- Explicit ILU pc matrix" << std::endl;

   cask::CsrMatrix csrPc{explicitPc.pc};

   std::vector<int> rows{0, 2, 3, 4, 6};
   std::vector<int> cols{0, 3, 1, 2, 0, 3};
   std::vector<double> vals{1, 1, 1, 1, 1, 1};
   ASSERT_EQ(csrPc.row_ptr, rows);
   ASSERT_EQ(csrPc.col_ind, cols);
   ASSERT_EQ(csrPc.values, vals);
}

TEST_F(TestLinearSolvers, ILUComputeAndApply) {
   cask::sparse_linear_solvers::ILUPreconditioner ilupc{cask::CsrMatrix{
       cask::DokMatrix{
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
   cask::utils::print(v,   "v            = ");
   cask::utils::print(res, "ILU.apply(v) = ");
   std::vector<double> expPcApply{-16.25, 7, 11, 15};
   cask::utils::print(expPcApply, "exp          = ");

   for (auto i = 0u; i < res.size(); i++) {
      ASSERT_DOUBLE_EQ(res[i], expPcApply[i]);
   }
}
