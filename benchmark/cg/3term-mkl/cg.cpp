#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>

#include "timer.hpp"

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

    This implements 3-term variant of CG (as in Hoemenn PhD thesis, page 225)

    x_{1}     <--- input, initial guess
    x_{0}     = 0
    r_{0}     = b
    r_{1}     = b - Ax_{1}
    w_{1}     = A*r_{1}

    ro_{1}    = 1
    mu_{1}    = (r_{1},r_{1})
    nu_{1}    = (w_{1},r_{1})
    gamma_{1} = mu_{1}/nu_{1}

    while (mu < tol^2) from k = 1
        x_{k+1}     = ro_{k}*(x_{k} + gamma_{k}*r_{k}) + (1-ro_{k})*x_{k-1}
        r_{k+1}     = ro_{k}*(r_{k} - gamma_{k}*w_{k}) + (1-ro_{k})*r_{k-1}
        w_{k+1}     = A*r_{k+1}
        mu_{k+1}    = (r_{k+1},r_{k+1})
        nu_{k+1}    = (w_{1+1},r_{k+1})
        gamma_{k+1} = mu_{k+1}/nu_{k+1}
        ro_{k+1}    = 1/( 1 - gamma_{k+1}/gamma_{k} * mu_{k+1}/mu_{k} * 1/ro_{k} );

    this way all dot products are postponed to the end of the loop.
*/
template <typename T>
bool cg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values,
        double* precon, double* rhs, double* sol,  bool verbose = false)
{
    Timer t;
    t.start();


    int size = n;
    int itercount = 0;
    int maxiter = 100; // maximum iterations 1000000

    long double rho, mu, nu, gamma;
    int k_offset, kp1_offset;

    // (symmetric) matrix is in lower-triangular form
    char tr = 'l';

    // NB: residual norm needs to be < this is tolerance^2
    // equivalent to tol = 1-e10 in mkl-rci benchmark
    double tolerance = 1e-5;    // relative error

    // Allocate array storage
    // Note double size for r and x: we need to store previous value for each.
    // Convention: x_{2k} starts at x[0], x_{2k+1} starts at x[size]. Same for r.
    std::vector<double>    w(size, 0.0);
    std::vector<double>    r(size*2, 0.0);
    std::vector<double>    x(size*2, 0.0);

    // x_{0} == 0 by declaration
    // x_{1} : let initial guess starts from vector [1,...,1]
    for (int i = 0; i < size; i++)
    {
        x[size + i] = 1.0;
    }

    // r_{1} = b - Ax_{1}
    mkl_dcsrsymv (&tr, &size, matrix_values, row_ptr, col_ind, &x[size], &r[size]); // r_{1}:=Ax_{1}
    cblas_daxpby(size, 1.0, &rhs[0], 1, -1.0, &r[size], 1);  // r_{1} = b - 1.0*r_{1} = b - Ax_{1}

    itercount = 1;
    // Continue until convergence.
    // Loop starts from computing x_{2}, r_{2}, ...
    while (true)
    {
        // we need cyclicly change the location where to write new vector results to
        // (we store 2 previous results but advance iteration by 1). Here's utility counter:
        k_offset   = size * ((itercount+1) % 2); // == 0 at first iteration (itercount == 1)
        kp1_offset = size * ((itercount+0) % 2); // first iteration computes x_{2} and r_{2}

        cout << "k_offset = " << k_offset << ", kp1_offset = " << kp1_offset << endl;

        // matrix multiply: w_{1} = A*r_{1};
        mkl_dcsrsymv (&tr, &size, matrix_values, row_ptr, col_ind, &r[kp1_offset], &w[0]);

        // scalar products and scalar factors
        //   mu_{k+1}    = (r_{k+1},r_{k+1})
        //   nu_{k+1}    = (w_{1+1},r_{k+1})
        //   gamma_{k+1} = mu_{k+1}/nu_{k+1}
        //   ro_{k+1}    = 1/( 1 - gamma_{k+1}/gamma_{k} * mu_{k+1}/mu_{k} * 1/ro_{k} );
        long double ex_mu = mu;
        long double ex_gamma = gamma;
        mu     = cblas_ddot(size, &r[kp1_offset], 1, &r[kp1_offset], 1);
        nu     = cblas_ddot(size, &r[kp1_offset], 1, &w[0], 1);
        gamma  = mu/nu;
        if (itercount == 1) rho = 1.0;
        else                rho = 1.0 / ( 1.0 - (gamma / ex_gamma) * (mu/ex_mu) * 1/rho );

        // test if norm is within tolerance
        if (mu < tolerance * tolerance)
        {
            t.stop();
            std::cout << "CG iterations made = " << itercount
                 << " in " << t.elapsed() << " sec"
                 << " using tolerance of "  << tolerance 
                 << " (error = " << std::sqrt(mu) << ")" 
                 << std::endl;
            break;
        }

        if(itercount > maxiter)
        {
            std::cout << "Exceeded maximum number of iterations: " << maxiter << std::endl;
//            print_array("r",   &r[0],   r.size());
//            print_array("sol", sol,     size);
            return false;
        }

        // Compute new search direction:
        //   x_{k+1}     = ro_{k}*(x_{k} + gamma_{k}*r_{k}) + (1-ro_{k})*x_{k-1}
        //   r_{k+1}     = ro_{k}*(r_{k} - gamma_{k}*w_{k}) + (1-ro_{k})*r_{k-1}
        for (int i = 0; i < size; i++)
        {
            x[k_offset + i] = (1-rho)*x[k_offset+i] + rho*(x[kp1_offset+i] + gamma*r[kp1_offset+i]);
            r[k_offset + i] = (1-rho)*r[k_offset+i] + rho*(r[kp1_offset+i] - gamma*w[i]);
        }

        if (verbose)
        {
            cout << " iteration " << itercount << ", (mu,nu,rho,gamma) = " << mu << ", " << nu << ", " << rho << ", " << gamma << endl;
            print_array("x", &x[0], x.size());
            print_array("r", &r[0], r.size());
            print_array("w", &w[0], w.size());
        }

        itercount++;

    } // while

    std::memcpy(sol, &x[k_offset], size*sizeof(T));

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

    bool verbose = true;
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
