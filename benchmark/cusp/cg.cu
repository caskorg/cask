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

int main(int argc, char ** argv) {

  array1d<ValueType, MemorySpace> rhs;

  if (argc < 3) {
    cout << "Usage: ./cg <path_to_matrix> <path_to_rhs>" << endl;
    return 1;
  }

  struct timeval ts_total, tf_total;
  struct timeval ts_compute, tf_compute;

  gettimeofday(&ts_total, NULL);
  cusp::csr_matrix<int, float, MemorySpace> A;
  cusp::io::read_matrix_market_file(A, argv[1]);
  cusp::array1d<ValueType, MemorySpace> bb(A.num_rows, 0);
  read_array_from_mm_file(bb, argv[2]);
  cusp::print(bb);
  cusp::print(A);

  // Solution vector
  cusp::array1d<ValueType, MemorySpace> x(A.num_rows, 0);

  // Set stopping criteria (iteration count, relative error)
  cusp::verbose_monitor<ValueType> monitor(bb, 512, 1e-3);

  // set preconditioner (identity)
  cusp::identity_operator<ValueType, MemorySpace> M(A.num_rows, A.num_rows);

  gettimeofday(&ts_compute, NULL);
  cusp::krylov::cg(A, x, bb, monitor, M);
  gettimeofday(&tf_compute, NULL);
  gettimeofday(&tf_total, NULL);
  long long elapsed = (tf_compute.tv_sec-ts_compute.tv_sec)*1000000 +
    tf_compute.tv_usec-ts_compute.tv_usec;

  long long elapsed_total = (tf_total.tv_sec-ts_total.tv_sec)*1000000 +
    tf_total.tv_usec-ts_total.tv_usec;

  printf("Compute time %f ms\n", elapsed/1000.0);
  printf("Total time %f s\n", elapsed_total/(1000.0 * 1000.0));

  return 0;
}
