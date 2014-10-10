#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>

#include "common.h"
#include "mmio.h"

// Intel MKL libraries
#include "mkl.h"
#include "mkl_blas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"



using namespace std;

/***
    solve A * x = b

    This implements reduced-communication reordering of  
    the standard PCG method (Demmel, Heath and Vorst, 1993)  

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

    this way all dot products are postponed to the end of the loop.
*/
template <typename T>
bool cg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values,
        double* precon, double* rhs, double* sol,  bool verbose = false)
{
    int size = n;
    int maxiter = 1000000; // maximum iterations

    // NB: residual norm needs to be < this is tolerance^2
    // equivalent to tol = 1-e10 in mkl-rci benchmark
    double tolerance = 1e-5;    // relative error


    // Allocate array storage
    std::vector<double>    w(size, 0.0);
    std::vector<double>    s(size, 0.0);
    std::vector<double>    p(size, 0.0);
    std::vector<double>    r(size, 0.0);
    std::vector<double>    q(size, 0.0);

    long double alpha, beta, rho_new;

    // Copy initial residual from input
    std::memcpy(&r[0], &rhs[0], size*sizeof(T));


    // evaluate initial residual error for exit check: eps = (r,r)
    T eps = cblas_dnrm2(size, &r[0], 1);

    // If input residual is less than tolerance skip solve.
    if (eps < tolerance * tolerance)
    {
        if (verbose)
        {
            std::cout << "CG iterations made = " << 0
                 << " using tolerance of "  << tolerance 
                 << " (error = " << std::sqrt(eps) << ")" << std::endl;
        }
        return true;
    }

    // apply diagonal preconditioner:  w = P*r;
    elementwise_xty(size, &precon[0], &r[0], &w[0]);

    // matrix multiply: s = A*w;
    char tr = 'l';
    mkl_dcsrsymv (&tr, &size, matrix_values, row_ptr, col_ind, &w[0], &s[0]);

    if (verbose)
    {
        print_array("r", &r[0], r.size());
        print_array("w", &w[0], w.size());
        print_array("s", &s[0], s.size());
    }

    // scalar products:
    //   rho = (r,w),
    //   mu = (s, w)
    T rho = cblas_ddot(size, &r[0], 1, &w[0], 1);;
    T mu  = cblas_ddot(size, &s[0], 1, &w[0], 1);;

    int itercount = 0;
    beta      = 0.0;
    alpha     = rho/mu;

    // Continue until convergence
    while (true)
    {
        if(itercount > maxiter)
        {
            std::cout << "Exceeded maximum number of iterations: " << maxiter << std::endl;

            print_array("r",   &r[0],   r.size());
            print_array("sol", sol,     size);

            return false;
        }

        // Compute new search direction:
        //   p = w + beta p
        //   q = s + beta q
        // NB: MKL proprietary extention to standard BLAS L1!
        // (daxpby implements y = a*x + b*y)
        cblas_daxpby(size, 1.0, &w[0], 1, beta, &p[0], 1);  // p = 1*w + beta*p
        cblas_daxpby(size, 1.0, &s[0], 1, beta, &q[0], 1);  // q = 1*s + beta*q

        // Update solution:
        //   x_{k+1} += alpha*p
        cblas_daxpy(size, alpha, &p[0], 1, &sol[0], 1);

        // Update residual vector
        //   r_{k+1} -= alpha*q
        cblas_daxpy(size, -alpha, &q[0], 1, &r[0], 1);

        // apply diagonal preconditioner:
        //   w = P*r;
        elementwise_xty(size, &precon[0], &r[0], &w[0]);

        // matrix multiply:
        //   s = A*w;
        char tr = 'l';
        mkl_dcsrsymv (&tr, &size, matrix_values, row_ptr, col_ind, &w[0], &s[0]);

        if (verbose)
        {
            print_array("p", &p[0], p.size());
            print_array("q", &q[0], q.size());
            print_array("r", &r[0], r.size());
            print_array("w", &w[0], w.size());
            print_array("s", &s[0], s.size());
            print_array("sol", sol, size);
        }

        // scalar products:
        //   rho = (r,w),
        //   mu  = (s,w)
        //   eps = (r,r)
        rho_new = cblas_ddot(size, &r[0], 1, &w[0], 1);
        mu      = cblas_ddot(size, &s[0], 1, &w[0], 1);
        eps     = cblas_ddot(size, &r[0], 1, &r[0], 1);

        // test if norm is within tolerance
        if (eps < tolerance * tolerance)
        {
            std::cout << "CG iterations made = " << itercount
                 << " using tolerance of "  << tolerance 
                 << " (error = " << std::sqrt(eps) << ")" 
                 << std::endl;
            break;
        }

        // Compute search direction and solution coefficients
        beta  = rho_new/rho;
        alpha = rho * rho_new * alpha / ( rho * mu * alpha - rho_new * rho_new );
        rho   = rho_new;
        itercount++;

    } // while

    return true;
}

// Solve an SPD sparse system using the conjugate gradient method with Intel MKL
int main (int argc, char** argv)
{
    FILE *f, *g;

    // read data from matrix market files
    if (argc < 3) {
      printf("Usage: ./cg <coeff_matrix> <rhs>\n");
      return -1;
    }

    if ((f = fopen(argv[1], "r")) == NULL) {
      printf("Could not open %s", argv[1]);
      return 1;
    }

    if ((g = fopen(argv[2], "r")) == NULL) {
      printf("Could not open %s", argv[2]);
      return 1;
    }

    printf("Read system matrix: ");
    int n, nnzs;
    double* values;
    int *col_ind, *row_ptr;
    read_system_matrix_sym_csr(f, &n, &nnzs, &col_ind, &row_ptr, &values);
    printf("n: %d, nnzs: %d\n", n, nnzs);

    printf("Read RHS: ");
    int vn, vnnzs;
    double* rhs = read_rhs(g, &vn, &vnnzs);
    //  print_array(rhs, vn);
    printf("n: %d, nnzs: %d\n", vn, vnnzs);

    fclose(f);
    fclose(g);


    // Initialise preconditioner; run through the matrix:
    //   if current nonzero entry is on diagonal, invert it
    std::vector<double>    precon(n, 1.0);
    int k = 0;
    for (int i = 0; i < nnzs; i++)
    {
        if (col_ind[i] == k+1)
        {
            precon[k++] = 1.0/values[i];
        }
    }


    // -------------------- Start solving the system ---------------------

    std::vector<double> sol(n, 0);

    bool verbose = false;
    bool status = cg<double>(n, nnzs, col_ind, row_ptr, values, &precon[0], rhs, &sol[0], verbose);


    printf ("Solution\n");
    print_array("sol = ", &sol[0], n);

    write_vector_to_file("sol.mtx.expl", &sol[0], sol.size());

    // check for expected solution
    /* for (i = 0; i < size && good; i++) */
    /*   good = fabs(expected_sol[i] - x[i]) < 1.0e-12; */
    //  print_array(x, size);


    mkl_free_buffers ();
    return 1;
}
