#include <cstdio>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <iterator>
#include "IO.hpp"
#include "common.h"
#include "mkl.h"
#include "BenchmarkUtils.hpp"

#include "lib/timer.hpp"

using namespace std;

/**
 *  Standard Nonpreconditioned CG, (Saad et al)
 *  https://en.wikipedia.org/wiki/Conjugate_gradient_method
 */
template <typename T>
bool cg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values,
        double* rhs, double* x,
        int& iterations, bool verbose = false)
{
    // configuration (TODO Should be exposed through params)
    char tr = 'l';
    int maxiters = 2000;
    double tol = 1E-5;

    std::vector<double>    r(n);             // residual
    std::vector<double>    b(rhs, rhs + n);  // rhs
    std::vector<double>    p(n);             //

    //  r = b - A * x
    mkl_dcsrsymv(&tr, &n, matrix_values, row_ptr, col_ind, &x[0], &r[0]);
    cblas_daxpby(n, 1.0, &b[0], 1, -1.0, &r[0], 1);
    p = r;

    double rsold = cblas_ddot(n, &r[0], 1, &r[0], 1);

    for (int i = 0; i < maxiters; i++) {
        if (verbose) {
            std::cout << " rsold " << rsold << std::endl;
        }
        std::vector<double> Ap(n);
        // Ap = A * p
        mkl_dcsrsymv (&tr, &n, matrix_values, row_ptr, col_ind, &p[0], &Ap[0]);
        // alpha = rsold / (p * Ap)
        double alpha = rsold / cblas_ddot(n, &p[0], 1, &Ap[0], 1);
        // x = x + alpha * p
        cblas_daxpy(n, alpha, &p[0], 1, &x[0], 1);
        // r = r - alpha * Ap
        cblas_daxpby(n, -alpha, &Ap[0], 1, 1.0, &r[0], 1);
        // rsnew = r * r
        double rsnew = cblas_ddot(n, &r[0], 1, &r[0], 1);

        if (rsnew <= tol * tol) {
            // std::cout << "Found solution" << std::endl;
            // print_array("x", &x[0], n);
            return true;
        }

        // p = r + (rsnew/rsold) * p
        cblas_daxpby(n, 1, &r[0], 1, rsnew / rsold, &p[0], 1);
        rsold = rsnew;
        iterations = i;
    }

    return false;
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv) {

    sparsebench::benchmarkutils::parseArgs(argc, argv);

    // read data from matrix market files
    int n, nnzs;
    double* values;
    int *col_ind, *row_ptr;
    FILE *f;
    if ((f = fopen(argv[2], "r")) == NULL) {
        printf("Could not open %s", argv[2]);
        return 1;
    }
    read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
    fclose(f);

    std::vector<double> rhs = sparsebench::io::readVector(std::string(argv[4]));

    std::vector<double> sol(n);

    bool verbose = false;
    int iterations = 0;

    sparsebench::utils::Timer t;
    t.tic("cg:all");
    bool status = cg<double>(n, nnzs, col_ind, row_ptr, values, &rhs[0], &sol[0], iterations, verbose);
    t.toc("cg:all");

    std::vector<double> exp = sparsebench::io::readVector(argv[6]);
    sparsebench::benchmarkutils::printSummary(
        0,
        iterations,
        t.get("cg:all").count(),
        0,
        sparsebench::benchmarkutils::residual(exp, sol),
        0
    );

    write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());

    mkl_free_buffers ();
    return 1;
}
