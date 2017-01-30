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

// which doubleing point type to use
typedef float ValueType;

// Reads an array from a Matrix Market sparse matrix file
void read_array_from_mm_file(array1d<ValueType,
    MemorySpace> &array, const char* filename,
    bool readAsMmFile) {
  ifstream f(filename);

  string line;

  while (getline(f, line)) {
    // this also skips the first line containing the matrix size
    if (line[0] != '%')
      break;
  }

  if (readAsMmFile) {
    while (!f.eof()) {
      int row, col;
      float val;
      f >> row >> col >> val;
      array[row - 1] = val;
    }
    return;
  }

  int nrows;
  istringstream(line) >> nrows;

  for (int i = 0; i < nrows; i++) {
    float val;
    f >> val;
    array[i] = val;
  }
}

int main(int argc, char ** argv) {

  array1d<ValueType, MemorySpace> rhs;

  if (argc < 3) {
    cout << "Usage: ./cg <path_to_matrix> <path_to_rhs> [-mm]" << endl;
    return 1;
  }

  cout << argc << endl;
  string flag("-mm");
  bool readAsMmFile = false;
  if (argc == 4) {
    readAsMmFile = (flag.compare(argv[3]) == 0);
  }

  printf("%s %s\n", argv[1], argv[2]);

  struct timeval ts_total, tf_total;
  struct timeval ts_compute, tf_compute;

  gettimeofday(&ts_total, NULL);
  printf("Loading matrix...\n");
  cusp::csr_matrix<int, float, MemorySpace> A;
  cusp::io::read_matrix_market_file(A, argv[1]);

  cusp::array1d<ValueType, MemorySpace> bb(A.num_rows, 0);
  printf("Reading RHS array...\n");

  read_array_from_mm_file(bb, argv[2], readAsMmFile);

  //  cusp::print(A);
  //  cusp::print(bb);
  printf("Read matrix A: %d rows %d cols\n", A.num_rows, 1);


  // Solution vector
  cusp::array1d<ValueType, MemorySpace> x(A.num_rows, 0);

  // Set stopping criteria (iteration count, relative error)
  cusp::verbose_monitor<ValueType> monitor(bb, 100 * A.num_rows, 1e-20);

  // set preconditioner (identity)
  cusp::identity_operator<ValueType, MemorySpace> M(A.num_rows, A.num_rows);


  printf("Starting cg...\n");
  gettimeofday(&ts_compute, NULL);
  cusp::krylov::cg(A, x, bb, monitor, M);
  gettimeofday(&tf_compute, NULL);
  gettimeofday(&tf_total, NULL);

  long long elapsed = (tf_compute.tv_sec-ts_compute.tv_sec)*1000000 +
    tf_compute.tv_usec-ts_compute.tv_usec;

  long long elapsed_total = (tf_total.tv_sec-ts_total.tv_sec)*1000000 +
    tf_total.tv_usec-ts_total.tv_usec;

  printf("Done...\n");
  printf("Compute time %f ms\n", elapsed/1000.0);
  printf("Total time %f s\n", elapsed_total/(1000.0 * 1000.0));

  ofstream g("sol.mtx");
  g << "%%MatrixMarket matrix array real general" << endl;
  g << "%-------------------------------------------------------------------------------" << endl;
  g << "1824 1" << endl;
  for (int i = 0; i < x.size(); i++)
    g << x[i] << endl;

  g.close();

  return 0;
}
