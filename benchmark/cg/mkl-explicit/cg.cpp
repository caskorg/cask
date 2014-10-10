#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include "common.h"
#include "mmio.h"

// Intel MKL libraries
#include "mkl_blas.h"
#include "mkl_spblas.h"
#include "mkl_service.h"



using namespace std;

/***

    This implements reduced-communication reordering of  
    the standard PCG method (Demmel, Heath and Vorst, 1993)  

    

*/
template <typename T>
bool cg(int n, int nnzs, int* col_ind, int* row_ptr, double* matrix_values, double* rhs, double* sol,
        bool verbose = false)
{
    /* printf("Matrix values\n"); */
    /* for (i = 0; i < nnzs; i ++) */
    /*   printf("%f ", values[i]); */
    /* printf("\n"); */
    /* for (i = 0; i < nnzs; i ++) */
    /*   printf("%d ", col_ind[i]); */
    /* printf("\n"); */
    /* for (i = 0; i < n + 1; i ++) *
    /*   printf("%d ", row_ptr[i]); */
    /* printf("\n"); */

    int size = n;
    int maxiter = 1000000; // maximum iterations

    // NB: residual norm needs to be < this is tolerance^2
    // equivalent to tol = 1-e10 in mkl-rci benchmark
    double tolerance = 1e-5;    // relative error


    // -------------------- Start solving the system ---------------------


    // Allocate array storage
    std::vector<double>    w(size, 0.0);
    std::vector<double>    s(size, 0.0);
    std::vector<double>    p(size, 0.0);
    std::vector<double>    r(size, 0.0);
    std::vector<double>    q(size, 0.0);

    std::vector<double>    precon(size, 1.0);

    T alpha, beta, rho_new;

    // Copy initial residual from input
    for (int i = 0; i < size; i++)
    {
        r[i] = rhs[i];
    }

    // Initialise preconditioner; run through the matrix:
    //   if current nonzero entry is on diagonal, invert it
    int k = 0;
    for (int i = 0; i < nnzs; i++)
    {
        printf("%d (%d) ", col_ind[i], i);
        if (col_ind[i] == k+1)
        {
            precon[k] = 1.0/matrix_values[i];
            printf("saving %d: matrix=%f, precon=%f |", col_ind[i],matrix_values[i],precon[k]);
            k++;
        }
        printf(" | ");
    }
    printf("precond: k = %d (size=%d)\n", k, size);

    // evaluate initial residual error for exit check
    T eps = 0;
    for (int i = 0; i < size; i++)
    {
        eps += r[i]*r[i];
    }

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
    for (int i = 0; i < size; i++)
    {
        w[i] = r[i]*precon[i];
    }

    // matrix multiply: s = A*w;
    char tr = 'l';
    mkl_dcsrsymv (&tr, &size, matrix_values, row_ptr, col_ind, &w[0], &s[0]);

    print_array("r", &r[0], r.size());
    print_array("w", &w[0], w.size());
    print_array("s", &s[0], s.size());

    T rho = 0.0;
    T mu  = 0.0;
    for (int i = 0; i < size; i++)
    {
        rho += r[i]*w[i];
        mu  += s[i]*w[i];
    }

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
            print_array("sol", sol, size);

            return false;
        }

        // -------------------------------------
        // TODO: substitute with BLAS L1 calls
        // -------------------------------------

        // Compute new search direction p_k, q_k
        for (int i = 0; i < size; i++)
        {
            p[i] = w[i] + beta*p[i];
            q[i] = s[i] + beta*q[i];
        }
        // Update solution x_{k+1}
        for (int i = 0; i < size; i++)
        {
            sol[i] += alpha*p[i];
        }
        // Update residual vector r_{k+1}
        for (int i = 0; i < size; i++)
        {
            r[i] -= alpha*q[i];
        }
        // Apply preconditioner
        for (int i = 0; i < size; i++)
        {
            w[i] = r[i]*precon[i];
        }

        // matrix multiply: s = A*w;
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

        rho_new = 0;
        mu      = 0;
        eps     = 0;
        for (int i = 0; i < size; i++)
        {
            rho_new += r[i]*w[i];
            mu      += s[i]*w[i];
            eps     += r[i]*r[i];
        }

        // test if norm is within tolerance
        if (eps < tolerance * tolerance)
        {
//            if (verbose)
            {
                std::cout << "CG iterations made = " << itercount
                     << " using tolerance of "  << tolerance 
                     << " (error = " << std::sqrt(eps) << ")" 
                     << std::endl;
            }
            break;
        }

        // Compute search direction and solution coefficients
        beta  = rho_new/rho;
        alpha = //rho_new/(mu - rho_new*beta/alpha);
                rho_new * alpha / ( mu * alpha - rho_new * beta );
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



    // -------------------- Start solving the system ---------------------

    std::vector<double> sol(n, 0);

    bool verbose = false;
    bool status = cg<double>(n, nnzs, col_ind, row_ptr, values, rhs, &sol[0], verbose);


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
