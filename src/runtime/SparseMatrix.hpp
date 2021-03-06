#ifndef SPARSEMATRIX_HPP_1BWMMLC8
#define SPARSEMATRIX_HPP_1BWMMLC8

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <vector>
#include <iostream>

#include <Eigen/Sparse>

namespace cask {
  namespace sparse {

    using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

    template<typename value_type>
  class SparkCooMatrix {

        public:
        using CoordType = std::tuple<int, int, value_type>;

        int n, m;
        std::vector<CoordType> data;

        SparkCooMatrix(int _n, int _m) : n(_n), m(_m) {}
      };
  }

// A dense vector representation
class Vector {
public:
  std::vector<double> data;

  Vector(int n) : data(n, 0) {}
  Vector(std::initializer_list<double> l) : data(l) {}
  Vector(const std::vector<double>& v) : data(v.begin(), v.end()) {}

  Vector operator-(const Vector& other) const {
    int n = size();
    if (other.size() != data.size()) {
      throw std::invalid_argument("Attempt to subtract vectors of different lengths: " +
                                  std::to_string(other.size()) + " != " + std::to_string(n));
    }
    Vector v(n);
    for (int i = 0; i < n; i++) {
      v[i] = data[i] - other[i];
    }
    return v;
  }

  int size() const {
    return data.size();
  }

  bool operator==(const Vector& other) const {
    return data == other.data;
  }

  const double& operator[](int i) const {
    return data[i];
  }

  double& operator[](int i) {
    return data[i];
  }

  void print(std::string label="") const {
    std::cout << label;
    for (int i : data)
      std::cout << i << " ";
    std::cout << std::endl;
  }

  double norm() const {
    double residual;
    for (double d : data)
      residual += d * d;
    return std::sqrt(residual);
  }

  double distance(const Vector& exp) const {
    return (*this - exp).norm();
  }

  void writeToFile(std::string path) {
    std::ofstream f{path};
    if (!f)
      throw std::invalid_argument("Could not open file for writing");
    for (auto v : data)
      f << v << std::endl;
  }
};

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
class DokMatrix {

 public:
  int n, m;
  int nnzs;

  // use of std::map important: values are sorted by column index internaly
  std::unordered_map<int, std::map<int, double>> dok;

  DokMatrix() : n(0), m(0), nnzs(0) {}

  DokMatrix(int _n, int m) : n(_n), m(m), nnzs(0) {}

  DokMatrix(int _n, int m, int _nnzs) : n(_n), m(m), nnzs(_nnzs) {}

  // Initialize the matrix as a dense matrix, with the given values, in order;
  // matrix is assumed square with n = sqrt(pattern.size()); 0 entries must be
  // entered explicitly
  DokMatrix(const std::initializer_list<double>& pattern) :
      DokMatrix(floor((sqrt(pattern.size()))), pattern) { }

  // Initialize the matrix as a dense matrix, with the given values, in order;
  // matrix is assumed of size n rows by pattern.size() / n columns;
  // 0 entries must be entered explicitly
  DokMatrix(int n, const std::initializer_list<double>& pattern) : n(n) {
    nnzs = std::count_if(pattern.begin(), pattern.end(), [](double x){return x != 0;});
    auto it = pattern.begin();
    m = pattern.size() / n;
    for (int i = 0; i < n; i++)
      for (int j = 0; j < m; j++) {
        double value = *it;
        if (value != 0) {
          dok[i][j] = value;
        }
        it++;
      }
  }

  DokMatrix explicitSymmetric() {
    int newNnzs = 0;
    DokMatrix m(n, this->m);
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
    return n == other.n && m == other.m && nnzs == other.nnzs && dok == other.dok;
  }

  void pretty_print() const {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < m; j++) {
        std::cout << at(i, j) << " ";
      }
      std::cout << "\n";
    }
  }

  double at(int i, int j) const {
    assert(i < n && j < m);
    if (dok.count(i) == 0)
      return 0;
    if (dok.at(i).count(j) == 0)
      return 0;
    return dok.at(i).at(j);
  }

  void set(int i, int j, double val) {
    assert(i < n && j < m);
    dok[i][j] = val;
    nnzs++;
  }

  bool isNnz(int i, int j) const {
    if (dok.count(i) == 0)
      return false;
    if (dok.at(i).count(j) == 0)
        return false;
    return dok.at(i).at(j) != 0;
  }

  DokMatrix getLowerTriangular() const {
    DokMatrix lowerTriangular(n, m);
    for (auto &e : dok) {
      for (auto &ee : e.second) {
        int i = e.first;
        int j = ee.first;
        double value = ee.second;
        if (j <= i)
          lowerTriangular.set(i, j, value);
      }
    }
    return lowerTriangular;
  }

  DokMatrix getUpperTriangular() const {
    DokMatrix lowerTriangular(n, m);
    for (auto &e : dok) {
      for (auto &ee : e.second) {
        int i = e.first;
        int j = ee.first;
        double value = ee.second;
        if (i <= j)
          lowerTriangular.set(i, j, value);
      }
    }
    return lowerTriangular;
  }

  Vector dot(const Vector& b) const {
    Vector result(b.size());
    for (auto &p : dok) {
      int row = p.first;
      for (auto &e : p.second) {
        result[row] += b[e.first] * e.second;
      }
    }
    return result;
  }

};

// TODO verify preconditions:
// with 1 based indexing
// lower triangular
// row entries in increasing column order
class CsrMatrix {
  // TODO move to include/, make API
 public:
  int n, m;
  int nnzs;
  std::vector<double> values;
  std::vector<int> col_ind;
  std::vector<int> row_ptr;

  CsrMatrix() : n(0), m(0), nnzs(0) {}

  CsrMatrix(std::initializer_list<double> mat) : CsrMatrix(DokMatrix(mat)){
  }

  CsrMatrix(int n, std::initializer_list<double> mat) : CsrMatrix(DokMatrix(n, mat)){
  }

  CsrMatrix(const DokMatrix &m) {
    nnzs = m.nnzs;
    this->n = m.n;
    this->m = m.m;
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

  CsrMatrix(int _n, int _m, int _nnzs, double *_values, int *_col_ind, int *_row_ptr) :
      n(_n), m(_m),
      nnzs(_nnzs) {
    values.assign(_values, _values + _nnzs);
    col_ind.assign(_col_ind, _col_ind + _nnzs);
    row_ptr.assign(_row_ptr, _row_ptr + n + 1);
  }

  CsrMatrix(int n, int m, int nnzs,
            const std::vector<double> &values,
            const std::vector<int> &col_ind,
            const std::vector<int> &row_ptr) :
      n(n), m(m), nnzs(nnzs), values(values), col_ind(col_ind), row_ptr(row_ptr) {}

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

  DokMatrix toDok() const {
    DokMatrix m(n, this->m, nnzs);
    for (int i = 0; i < n; i++) {
      for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
        m.dok[i][col_ind[k]] = values[k];
      }
    }
    return m;
  }

  bool operator==(const CsrMatrix& o) const {
    return n == o.n &&
        m == o.m &&
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

  CsrMatrix getLowerTriangular() const {
    return CsrMatrix(toDok().getLowerTriangular());
  }

  CsrMatrix getUpperTriangular() const {
    return CsrMatrix(toDok().getUpperTriangular());
  }

  Vector dot(const Vector& b) const {
    return toDok().dot(b);
  }

  CsrMatrix sliceRows(int startRow, int nRows) const {
    //startPos = row_ptr[startRow];
    std::vector<int> newRowPtr, newColInd;
    std::vector<double> newValues;
    int nnzs = 0;
    int offset = row_ptr[startRow];
    for (int i = startRow; i < startRow + nRows; i++) {
      int r = row_ptr[i];
      newRowPtr.push_back(r - offset);
      for (int j = r; j < row_ptr[i + 1]; j++) {
        newValues.push_back(values[j]);
        newColInd.push_back(col_ind[j]);
        nnzs++;
      }
    }
    newRowPtr.push_back(newValues.size());
    return CsrMatrix(nRows, this->m, nnzs, newValues, newColInd, newRowPtr);
  }


  /** Slices columns from this matrix into blocks. Each block has as
      many rows as this matrix but at most blockSize columns.  All
      blocks except the last one have exactly blockSize columns.

      Example:

      This matrix:
      1 2 3 4
      5 6 7 8
      sliceColumns(3) returns:
      1 2 3 | 4
      5 6 7 | 8
  */
  std::vector<cask::CsrMatrix> sliceColumns(int blockSize) const {
    // XXX constructing a CSR matrix this way is not safe because m and n are not updated
    int nBlocks = m / blockSize + (m % blockSize == 0 ? 0 : 1);
    std::vector<cask::CsrMatrix> partitions(nBlocks);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < nBlocks; j++) {
        auto& p = partitions[j].row_ptr;
        if (p.size() == 0)
          p.push_back(0);
        else
          p.push_back(p.back());
      }
      //std::cout << "i = " << i << std::endl;
      //std::cout << "colptr" << colptr[i] << std::endl;
      for (int j = row_ptr[i]; j < row_ptr[i+1]; j++) {
        auto& p = partitions[col_ind[j] / blockSize];
        int idxInPartition = col_ind[j] - (col_ind[j] / blockSize ) * blockSize;
        p.col_ind.push_back(idxInPartition);
        p.values.push_back(values[j]);
        p.row_ptr.back()++;
      }
    }
    return partitions;
  }

};

/** A symmetric matrix for which only the lower triangle is stored explicitly, in CSR format */
class SymCsrMatrix {
 public:
  int n, m;
  int nnzs;
  CsrMatrix matrix;

  // Construct a symmetric matrix from a lower triangular matrix in DoK format
  explicit SymCsrMatrix(const DokMatrix& l) : n(l.n), m(l.m), matrix(l) {
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

  Vector dot(const Vector& b) const {
    return matrix.toDok().explicitSymmetric().dot(b);
  }

};

}

#endif /* end of include guard: SPARSEMATRIX_HPP_1BWMMLC8 */
