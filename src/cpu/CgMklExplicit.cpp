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

using namespace std;

/**
    Solve A * x = b using the reduced-communication reordering of  
    the PCG method (Demmel, Heath and Vorst, 1993)  

    r_{0}     = b
    w_{0}     = P*r_{0}
    s_{0}     = A*w_{0}
    eps_{0}   = (r_{0},r_{0})
    rho_{0}   = (r_{0},w_{0})
    mu_{0}    = (s_{0},w_{0})
    alpha_{0} = rho_{0}/mu_{0}
    beta_{0}  = 0

    while (eps < tol^2) from k = 1
        p_{k} = w_{k-1} + beta_{k-1} p_{k-1}
        q_{k} = s_{k-1} + beta_{k-1} q_{k-1}
        x_{k} += alpha_{k-1}*p_{k}
        r_{k} -= alpha_{k-1}*q_{k}
        w_{k} = P*r_{k}
        s_{k} = A*w_{k}

        rho_{k} = (r_{k},w_{k})
        mu_{k}  = (s_{k},w_{k})
        eps_{k} = (r_{k},r_{k})
        beta_{k}  = rho_{k}/rho_{k-1};
        alpha_{k} = rho_{k} / ( mu_{k} - rho_{k} * beta_{k}/alpha_{k-1} );
*/
template <typename T>
bool cg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values,
        double* precon, double* rhs, double* sol,
        int& iterations, bool verbose = false)
{
    char tr = 'l';
    int maxiter = 1000000;

    // NB: residual norm needs to be < this is tolerance^2
    // equivalent to tol = 1-e10 in mkl-rci benchmark
    double tolerance = 1e-5;    // relative error

    // Allocate array storage
    std::vector<double>    w(n, 0.0);
    std::vector<double>    s(n, 0.0);
    std::vector<double>    p(n, 0.0);
    std::vector<double>    r(rhs, rhs + n);
    std::vector<double>    q(n, 0.0);

    long double alpha, beta, rho_new;

    T eps = cblas_dnrm2(n, &r[0], 1);

    if (eps < tolerance * tolerance) {
        return true;
    }
    elementwise_xty(n, &precon[0], &r[0], &w[0]);
    mkl_dcsrsymv(&tr, &n, matrix_values, row_ptr, col_ind, &w[0], &s[0]);
    T rho = cblas_ddot(n, &r[0], 1, &w[0], 1);;
    T mu  = cblas_ddot(n, &s[0], 1, &w[0], 1);;

    beta      = 0.0;
    alpha     = rho/mu;

    // Continue until convergence
    while (true) {
        if(iterations > maxiter) {
            std::cout << "Exceeded maximum number of iterations: " << maxiter << std::endl;
            return false;
        }

        cblas_daxpby(n, 1.0, &w[0], 1, beta, &p[0], 1);
        cblas_daxpby(n, 1.0, &s[0], 1, beta, &q[0], 1);
        cblas_daxpy(n, alpha, &p[0], 1, &sol[0], 1);
        cblas_daxpy(n, -alpha, &q[0], 1, &r[0], 1);
        elementwise_xty(n, &precon[0], &r[0], &w[0]);
        mkl_dcsrsymv (&tr, &n, matrix_values, row_ptr, col_ind, &w[0], &s[0]);
        rho_new = cblas_ddot(n, &r[0], 1, &w[0], 1);
        mu      = cblas_ddot(n, &s[0], 1, &w[0], 1);
        eps     = cblas_ddot(n, &r[0], 1, &r[0], 1);

        // test if norm is within tolerance
        if (eps < tolerance * tolerance) {
            break;
        }

        // Compute search direction and solution coefficients
        beta  = rho_new/rho;
        alpha = rho * rho_new * alpha / ( rho * mu * alpha - rho_new * rho_new );
        rho   = rho_new;
        iterations++;
    }

    return true;
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

    std::vector<double> precon(n, 1.0);
    int k = 0;
    for (int i = 0; i < nnzs; i++) {
        if (col_ind[i] == k+1) {
            precon[k++] = 1.0/values[i];
        }
    }

    std::vector<double> sol(n, 0);

    bool verbose = false;
    int iterations;
    bool status = cg<double>(n, nnzs, col_ind, row_ptr, values, &precon[0], &rhs[0], &sol[0], iterations, verbose);

    std::vector<double> exp = sparsebench::io::readVector(argv[6]);
    sparsebench::benchmarkutils::printSummary(
        0,
        iterations,
        0,
        0,
        sparsebench::benchmarkutils::residual(exp, sol),
        0
    );

    write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());

    mkl_free_buffers ();
    return 1;
}
