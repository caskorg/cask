#ifndef SPAM_SPARSEMATRIX_HPP
#define SPAM_SPARSEMATRIX_HPP

namespace spam {

class DokMatrix {

 public:
  int n;
  int nnzs;

  // use of std::map important: values are sorted by column index internaly
  std::unordered_map<int, std::map<int, double>> dok;

  DokMatrix(int _n, int _nnzs) : n(_n), nnzs(_nnzs) {}


  DokMatrix explicitSymmetric() {
    DokMatrix m(n, nnzs);
    for (auto &&s : dok) {
      for (auto &&p : s.second) {
        int i = s.first;
        int j = p.first;
        double value = p.second;
        m.dok[i][j] = value;

        // check the transpose entry does not exist
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
      }
    }
    return m;
  }

  void pretty_print() {
    for (auto &&s : dok) {
      for (auto &&p : s.second) {
        std::cout << s.first << " " << p.first << " " << p.second << std::endl;
      }
    }
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

  CsrMatrix(const DokMatrix &m) {
    nnzs = m.nnzs;
    n = m.n;
    // row_ptr.resize(n + 1);
    int pos = 1;
    for (int i = 1; i < n + 1; i++) {
      row_ptr.push_back(pos);
      for (auto &entries : m.dok.find(i)->second) {
        col_ind.push_back(entries.first);
        values.push_back(entries.second);
        pos++;
      }
    }
    row_ptr.push_back(nnzs + 2);
  }

  CsrMatrix(int _n, int _nnzs, double *_values, int *_col_ind, int *_row_ptr) :
      n(_n),
      nnzs(_nnzs) {
    values.assign(_values, _values + _nnzs);
    col_ind.assign(_col_ind, _col_ind + _nnzs);
    row_ptr.assign(_row_ptr, _row_ptr + n + 1);
  }

  // Prints all matrix values
  void pretty_print() {
    for (int i = 0; i < n; i++) {
      int col_ptr = row_ptr[i];
      for (int k = 0; k < n; k++) {
        if (col_ptr < row_ptr[i + 1] && col_ind[col_ptr - 1] == k + 1) {
          std::cout << values[col_ptr - 1] << " ";
          col_ptr++;
          continue;
        }
        std::cout << "0 ";
      }
      std::cout << "\n";
    }
  }

  double &get(int i, int j) {
    for (int k = row_ptr[i - 1]; k < row_ptr[i]; k++) {
      if (col_ind[k - 1] == j) {
        return values[k - 1];
      }
    }

    throw std::invalid_argument("No nonzero at row col:"
                                    + std::to_string(i) + " "
                                    + std::to_string(j));
  }

  bool isNnz(int i, int j) {
    try {
      get(i, j);
    } catch (std::invalid_argument) {
      return false;
    }
    return true;
  }

  bool isSymmetric() {
    return true;
  }

  DokMatrix toDok() {
    DokMatrix m(n, nnzs);
    for (int i = 0; i < n; i++) {
      for (int k = row_ptr[i]; k < row_ptr[i + 1]; k++) {
        m.dok[i + 1][col_ind[k - 1]] = values[k - 1];
      }
    }
    return m;
  }
};

}
#endif //SPAM_SPARSEMATRIX_HPP
