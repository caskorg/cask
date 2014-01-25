#include <cusp/hyb_matrix.h>
#include <cusp/gallery/poisson.h>
#include <cusp/krylov/cg.h>
#include <sys/time.h>

#include <cusp/io/matrix_market.h>
#include <cusp/coo_matrix.h>
#include <cusp/print.h>

#include <iostream>

using namespace std;
using namespace cusp;
// where to perform the computation
typedef cusp::device_memory MemorySpace;

// which floating point type to use
typedef float ValueType;

// Reads an array from a Matrix Market sparse matrix file
void read_array_from_mm_file(array1d<ValueType, MemorySpace> &array, const char* filename) {
  ifstream f(filename);

  string line;

  while (getline(f, line)) {
    if (line[0] != '%')
      break;
  }

  while (!f.eof()) {
    int row, col;
    float val;
    f >> row >> col >> val;
    array[row] = val;
  }

  cout << line << endl;
}

int main(int argc, char ** argv)
{

  array1d<ValueType, MemorySpace> rhs;
  bool allZeroRhs = false;

  if (argc < 2) {
    cout << "Usage: ./cg <path_to_matrix> [<path_to_rhs>]" << endl;
    return 1;
  } else if (argc == 2) {
    allZeroRhs = true;
  }


  struct timeval ts_total, tf_total;
  struct timeval ts_compute, tf_compute;

  printf("%s\n", argv[1]);

  cusp::csr_matrix<int, float, MemorySpace> A;
  // TODO read b in host memory first
  cusp::io::read_matrix_market_file(A, argv[1]);


  int rhs_size = A.num_rows;
  cusp::array1d<ValueType, MemorySpace> bb(rhs_size, 0);
  //  cusp::io::read_matrix_market_file(bb, argv[1]);

  read_array_from_mm_file(bb, argv[2]);
  cusp::print(bb);
  return;


  cusp::array1d<ValueType, MemorySpace> x(A.num_rows, 0);


  // TODO need to convert b to array1d

  cusp::print(A);


  cusp::identity_operator<ValueType, MemorySpace> M(A.num_rows, A.num_rows);
  cusp::verbose_monitor<ValueType> monitor(bb, 512, 1e-3);

  cusp::krylov::cg(A, x, bb, monitor, M);

  /* gettimeofday(&ts_total, NULL); */
  /* // create an empty sparse matrix structure (HYB format) */
  /* cusp::hyb_matrix<int, ValueType, MemorySpace> A; */

  /* // create a 2d Poisson problem on a 10x10 mesh */
  /* gettimeofday(&ts_compute, NULL); */
  /* cusp::gallery::poisson5pt(A, 512, 512); */
  /* gettimeofday(&tf_compute, NULL); */

  /* long elapsed = (tf_compute.tv_sec-ts_compute.tv_sec)*1000000 + */
  /*   tf_compute.tv_usec-ts_compute.tv_usec; */
  /* printf("Poisson compute time %d.%d ms\n", elapsed/1000); */

  /* // allocate storage for solution (x) and right hand side (b) */
  /* cusp::array1d<ValueType, MemorySpace> x(A.num_rows, 0); */
  /* cusp::array1d<ValueType, MemorySpace> b(A.num_rows, 1); */

  /* // set stopping criteria: */
  /* // iteration_limit = 100 */
  /* // relative_tolerance = 1e-3 */
  /* cusp::verbose_monitor<ValueType> monitor(b, 512, 1e-3); */

  /* // set preconditioner (identity) */
  /* cusp::identity_operator<ValueType, MemorySpace> M(A.num_rows, A.num_rows); */


  /* gettimeofday(&ts_compute, NULL); */
  /* cusp::krylov::cg(A, x, b, monitor, M); */
  /* gettimeofday(&tf_compute, NULL); */

  /* gettimeofday(&tf_total, NULL); */

  /* elapsed = (tf_compute.tv_sec-ts_compute.tv_sec)*1000000 + */
  /*   tf_compute.tv_usec-ts_compute.tv_usec; */

  /* long elapsed_total = (tf_total.tv_sec-ts_total.tv_sec)*1000000 + */
  /*   tf_total.tv_usec-ts_total.tv_usec; */

  /* printf("ts_compute.sec: %d ts_compute.usec: %d\n", */
  /*        ts_compute.tv_sec, ts_compute.tv_usec); */
  /* printf("tf_compute.sec: %d tf_compute.usec: %d\n", */
  /*        tf_compute.tv_sec, tf_compute.tv_usec); */

  /* printf("Compute time %d.%d ms\n", elapsed/1000); */
  /* printf("Total time %d.%d ms\n", elapsed_total/1000); */



  /* return 0; */
}
