#ifndef SPAM_SPARSEMATRIX_HPP
#define SPAM_SPARSEMATRIX_HPP

#include <unordered_map>
#include <map>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <fstream>

/**
 * Sparse matrix representations for some common storage formats:
 * - DoK (Dictionary of Keys) -- the de facto format for all/many construction tasks; random read/write access is
 * efficient, but memory footprint is large
 * - CSR -- an efficient format for when only row slicing operations are required; random read access is supported but inefficient,
 * random write access is only supported in update mode (cannot insert new nonzeros), memory footprint is minimal
 *
 * Some underlying assumptions that are respected by all matrix formats:
 *
 * - all storage is 0 indexed; it is incredibly awkward and error-prone to support 1-based indexing in a language like C++;
 * - for interfacing with certain codes that do not support 0 based indexing (e.g. if using BLAS from Fortran), some
 * formats provide a getIndexed() method to obtain the corresponding representation in 1 based indexing. Read the specific
 * of each method to understand the storage format and prevent bugs when interfacing with low-level C/Fortran APIs
 *
 * TODOs
 * - a consistent, safe mechanism for handling symmetric matrices?
 */
namespace spam {

class DokMatrix {

 public:
  int n;
  int nnzs;

  // use of std::map important: values are sorted by column index internaly
  std::unordered_map<int, std::map<int, double>> dok;

  DokMatrix(int _n) : n(_n), nnzs(0) {}

  DokMatrix(int _n, int _nnzs) : n(_n), nnzs(_nnzs) {}

  // Initialize the matrix as a dense matrix, with the given values, in order;
  // matrix is assumed square with n = sqrt(pattern.size()); 0 entries must be
  // entered explicitly
  DokMatrix(const std::initializer_list<double>& pattern) {
    n = floor((sqrt(pattern.size())));
    nnzs = std::count_if(pattern.begin(), pattern.end(), [](double x){return x != 0;});
    auto it = pattern.begin();
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++) {
        double value = *it;
        if (value != 0)
          dok[i][j] = value;
        it++;
      }
  }

  DokMatrix explicitSymmetric() {
    int newNnzs = 0;
    DokMatrix m(n);
    for (auto &&s : dok) {
      for (auto &&p : s.second) {
        int i = s.first;
        int j = p.first;
        double value = p.second;
        m.dok[i][j] = value;
        newNnzs++;

        // check the transpose entry does not exist
        // TODO replace dok.find with dok.at
        if (i == j)
          continue;
        if (dok.find(j) != dok.end()) {
          if (dok[j].find(i) != dok[j].end()) {
            if (dok[j][i] != p.second) {
              throw std::invalid_argument("Matrix is not symmetric");
            } else {
              std::cout << "Warning! Matrix already contains transpose entry for "
                        << s.first << " " << p.first << std::endl;
            }
          }
        }

        // add the transpose entry
        m.dok[j][i] = value;
        newNnzs++;
      }
    }
    m.nnzs = newNnzs;
    return m;
  }

  bool operator==(const DokMatrix& other) const {
    return n == other.n && nnzs == other.nnzs && dok == other.dok;
  }

  void pretty_print() const {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        std::cout << at(i, j) << " ";
      }
      std::cout << "\n";
    }
  }

  double at(int i, int j) const {
    assert(i < n && j < n);
    if (dok.count(i) == 0)
      return 0;
    if (dok.at(i).count(j) == 0)
      return 0;
    return dok.at(i).at(j);
  }

  void set(int i, int j, double val) {
    assert(i < n && j < n);
    dok[i][j] = val;
    nnzs++;
  }
};

// TODO verify preconditions:
// with 1 based indexing
// lower triangular
// row entries in increasing column order
class CsrMatrix {
  // TODO move to include/, make API
 public:
  int n;
  int nnzs;
  std::vector<double> values;
  std::vector<int> col_ind;
  std::vector<int> row_ptr;

  CsrMatrix() : n(0), nnzs(0) {}

  explicit CsrMatrix(const DokMatrix &m) {
    nnzs = m.nnzs;
    n = m.n;
    int pos = 0;
    for (int i = 0; i < n; i++) {
      row_ptr.push_back(pos);
      if (m.dok.count(i) != 0) {
        for (auto &entries : m.dok.at(i)) {
          col_ind.push_back(entries.first);
          values.push_back(entries.second);
          pos++;
        }
      }
    }
    row_ptr.push_back(nnzs);
  }

  CsrMatrix(int _n, int _nnzs, double *_values, int *_col_ind, int *_row_ptr) :
      n(_n),
      nnzs(_nnzs) {
    values.assign(_values, _values + _nnzs);
    col_ind.assign(_col_ind, _col_ind + _nnzs);
    row_ptr.assign(_row_ptr, _row_ptr + n + 1);
  }

  // Prints all matrix values
  void pretty_print() const {
    for (int i = 0; i < n; i++) {
      int col_ptr = row_ptr[i];
      for (int j = 0; j < n; j++) {
        if (col_ptr < row_ptr[i + 1] && col_ind[col_ptr] == j) {
          std::cout << values[col_ptr] << " ";
          col_ptr++;
          continue;
        }
        std::cout << "0 ";
      }
      std::cout << "\n";
    }
  }

  void print() {
    std::cout << "CSRMatrix( n= " << n << " nnzs= " << nnzs << ")" << std::endl;
    std::cout << "values = ";
    std::copy(values.begin(), values.end(), std::ostream_iterator<double>{std::cout, " "});
    std::cout << std::endl;
    std::cout << "col_ind = ";
    std::copy(col_ind.begin(), col_ind.end(), std::ostream_iterator<double>{std::cout, " "});
    std::cout << std::endl;
    std::cout << "row_ptr = ";
    std::copy(row_ptr.begin(), row_ptr.end(), std::ostream_iterator<double>{std::cout, " "});
    std::cout << std::endl;
  }

  double &get(int i, int j) {
    for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
      if (col_ind[k] == j) {
        return values[k];
      }
    }

    throw std::invalid_argument("No nonzero at row col:"
                                    + std::to_string(i) + " "
                                    + std::to_string(j));
  }

  bool isNnz(int i, int j) {
    // TODO efficiency could be improved
    try {
      get(i, j);
    } catch (std::invalid_argument) {
      return false;
    }
    return true;
  }

  bool isSymmetric() const {
    return true;
  }

  DokMatrix toDok() {
    DokMatrix m(n, nnzs);
    for (int i = 0; i < n; i++) {
      for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
        m.dok[i][col_ind[k]] = values[k];
      }
    }
    return m;
  }

  bool operator==(const CsrMatrix& o) const {
    return n == o.n &&
        nnzs == o.nnzs &&
        values == o.values &&
        row_ptr == o.row_ptr &&
        col_ind == o.col_ind;
  }

  std::vector<int> getRowPtrWithOneBasedIndex() const {
    std::vector<int> rowIndexed;
    std::transform(
        row_ptr.begin(), row_ptr.end(), std::back_inserter(rowIndexed),
        [](int x){return x + 1;}
    );
    return rowIndexed;
  }

  std::vector<int> getColIndWithOneBasedIndex() const {
    std::vector<int> colIndexed;
    std::transform(
        col_ind.begin(), col_ind.end(), std::back_inserter(colIndexed),
        [](int x){return x + 1;}
    );
    return colIndexed;

  }

  CsrMatrix getLowerTriangular() {
    DokMatrix lowerTriangular(n);
    for (int i = 0; i < n; i++) {
      for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
        int j = col_ind[k];
        double value = values[k];
        if (j <= i)
          lowerTriangular.set(i, j, value);
      }
    }
    return CsrMatrix(lowerTriangular);
  }

  CsrMatrix getUpperTriangular() {
    DokMatrix upperTriangular(n);
    for (int i = 0; i < n; i++) {
      for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
        int j = col_ind[k];
        double value = values[k];
        if (i <= j)
          upperTriangular.set(i, j, value);
      }
    }
    return CsrMatrix(upperTriangular);
  }

};

/** A symmetric matrix for which only the lower triangle is stored explicitly, in CSR format */
class SymCsrMatrix {
 public:
  int n;
  int nnzs;
  CsrMatrix matrix;

  // Construct a symmetric matrix from a lower triangular matrix in DoK format
  explicit SymCsrMatrix(const DokMatrix& l) : n(l.n), matrix(l) {
    int diagNnzs = 0;
    for (int i = 0; i < l.n; i++)
      if (l.at(i, i) != 0)
        diagNnzs++;
    nnzs = 2 * (l.nnzs - diagNnzs) + diagNnzs;
  }

  void print() {
    // matrix.print();
  }

  void pretty_print() {
    std::cout << "Stored matrix: " << std::endl;
    matrix.pretty_print();
    std::cout << "Implicit values: " << std::endl;
    matrix.toDok().explicitSymmetric().pretty_print();
  }
};

void writeToFile(std::string path, std::vector<double> vector) {
  std::ofstream f{path};
  if (!f)
    throw std::invalid_argument("Could not open file for writing");
  for (auto v : vector)
    f << v << std::endl;
}

}
#endif //SPAM_SPARSEMATRIX_HPP
