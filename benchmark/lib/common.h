#ifndef COMMON_BENCHMARK_H
#define COMMON_BENCHMARK_H

#include "mmio.h"


void print_array(double *values, int size);
void print_array_int(int *values, int size);
void print_matrix(double *values, int size);

/** Reads the dimensions of the matrix (n - number of rows, m - number
    of columns, nnzs - number of non-zeros) from the given Matrix
    Market file.  If the given file contains an array rather than a
    COO matrix, nnzs will be set to n;
*/
void read_mm_matrix_size(FILE *f, int *n, int *m, int *nnzs, MM_typecode* mcode);

/** Reads a matrix market file for a symmetric real valued sparse
    matrix and returns the matrix in 0-indexed CSR form. */
void read_mm_sym_matrix(FILE* f, MM_typecode mcode,
                        int n, int nnzs,
                        double *values, int* col_ind, int *row_ptr
                        );

/** Returns the zero indexed array of values. */
void read_mm_array(FILE *f, MM_typecode code, int nnzs, double *values);
double* read_rhs(FILE* g, int* n, int *nnzs);
void read_system_matrix_sym_csr(FILE* f, int* n, int *nnzs, int** col_ind, int** row_ptr, double** values);
void write_vector_to_file(const char* filename, double* vector, int size);


#endif