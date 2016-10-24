#include <vector>
#include <Benchmark.hpp>
#include <LinearSolvers.hpp>
#include <IO.hpp>
#include <SparseMatrix.hpp>
#include <Utils.hpp>
#include <gtest/gtest.h>

class TestLinearSolvers : public ::testing::Test {

};

TEST_F(TestLinearSolvers, CGWithIdentityPC) {
   std::vector<double> rhs = spam::io::readVector("test/systems/tiny_b.mtx");
   spam::CsrMatrix a = spam::io::readMatrix("test/systems/tiny.mtx");

   int iterations = 0;
   std::vector<double> sol(a.n);

   spam::Timer t;
   t.tic("cg:all");
   bool converged = spam::pcg<double, spam::IdentityPreconditioner>(a, &rhs[0], &sol[0], iterations);
   t.toc("cg:all");

   std::vector<double> exp = spam::io::readVector("test/systems/tiny_sol.mtx");
   spam::benchmark::printSummary(
       0,
       iterations,
       t.get("cg:all").count(),
       0,
       spam::benchmark::residual(exp, sol),
       0
   );

   write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());
   // TODO verify contents
   mkl_free_buffers ();

   ASSERT_TRUE(converged);
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
   // TODO verify contents
}
