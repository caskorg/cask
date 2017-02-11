#ifndef CASK_IO_HPP
#define CASK_IO_HPP

#include <string>
#include <iostream>
#include "SparseMatrix.hpp"
#include "SparseMatrix.hpp"
#include <dfesnippets/Timing.hpp>
#include <regex>
#include <fstream>
#include <cassert>
#include <sstream>

namespace cask {

/** I/O facilities for various matrix storage formats */
namespace io {

/** Support for Matrix Market I/O. http://math.nist.gov/MatrixMarket/formats.html
 *
 * Matrix Market format is:
 *
 * %%MatrixMarket <type> <format> <data type> <symmetry>
 * % Any number of comment lines
 * N M L
 * i_1 j_1 v_1
 * . . . . . .
 * i_L j_L v_L
 *
 * where
 * type = matrix
 * format = coordinate | array
 * data type = real | complex | integer | pattern
 * symmetry = symmetric | skew-symmetric | hermitian | general
 *
 * Note:
 *   pattern, complex, skew-symmetric, hermitian are not supported will throw an exception
 */
struct MmInfo {
  const std::string type;
  const std::string format;
  const std::string dataType;
  const std::string symmetry;
  MmInfo(std::string _type, std::string _format, std::string _dataType, std::string _symmetry) :
      type(_type),
      format(_format),
      dataType(_dataType),
      symmetry(_symmetry) {}
  bool isMatrix() const {
    return type == "matrix";
  }
  bool isSymmetric() const {
    return symmetry == "symmetric";
  }
  bool isCoordinate() {
    return format == "coordinate";
  }
};

inline MmInfo readHeader(std::string path) {
  std::ifstream f{path};
  if (!f)
    throw std::invalid_argument("File not found " + path);
  std::string fileInfo;
  std::getline(f, fileInfo);
  std::regex mmHeaderRe("%%MatrixMarket (matrix|array) (coordinate|array) (real|integer) (symmetric|general)");
  std::smatch m;
  if (regex_match(fileInfo, m, mmHeaderRe))
    return MmInfo{m[1], m[2], m[3], m[4]};
  throw std::invalid_argument("Not a valid MatrixMarket file in " + path);
}

inline std::vector<double> readVector(std::string path) {
  MmInfo info = readHeader(path);

  assert(info.isMatrix());
  assert(info.symmetry == "general");
  assert(info.dataType == "real");

  // skip header
  std::ifstream f{path};
  std::string line;
  while (std::getline(f, line)) {
    if (line[0] != '%')
      break;
  }

  int n, m;
  std::stringstream s;
  s << line;
  s >> n >> m;
  std::vector<double> v(n);

  if (info.format == "coordinate") {
    int l;
    s >> l;
    for (int i = 0; i < l; i++) {
      int a, b;
      double val;
      f >> a >> b >> val;
      v[a] = val;
    }
    return v;
  }

  assert(info.format == "array");

  s >> n >> m;
  for (int i = 0; i < n; i++) {
    double val;
    f >> val;
    v[i] = val;
  }
  return v;
}

inline cask::CsrMatrix readMatrix(std::string path) {
  MmInfo info = readHeader(path);
  if (!info.isMatrix()) {
    throw std::invalid_argument("Error! Expecting MatrixMarket matrix in " + path);
  }

  if (info.isSymmetric()) {
    throw std::invalid_argument("Error! Symmetric Matrix found in " +
        path + " To read symmetric matrix use cask::io::readSymMatrix()");
  }

  throw std::invalid_argument("Error! Generic matrix in MatrixMarket not supported");
}

inline cask::SymCsrMatrix readSymMatrix(std::string path) {
  MmInfo info = readHeader(path);
  if (!info.isMatrix()) {
    throw std::invalid_argument("Error! Expecting MatrixMarket matrix in " + path);
  }

  if (!info.isSymmetric()) {
    throw std::invalid_argument("Error! Matrix found in " + path +
        " is not symmetric. To read unsymmetric matrix use cask::io::readSymMatrix()");
  }

  std::ifstream f{path};
  std::string line;
  while (std::getline(f, line)) {
    if (line[0] != '%')
      break;
  }

  assert(info.isCoordinate());

  int n, m, l;
  std::stringstream ss;
  ss << line;
  ss >> n >> m >> l;
  cask::DokMatrix mat(n);

  for (int k = 0; k < l; k++) {
    int i, j;
    double val;
    f >> i >> j >> val;
    // MM format is 1-indexed
    mat.set(i - 1, j - 1, val);
  }

  return cask::SymCsrMatrix(mat);
}

template<typename value_type>
class MmReader {

    bool sparse = false, symmetric = false, matrix = false;
    int ncols, nrows, nnzs = -1;

    using SparseMatrix = cask::sparse::SparkCooMatrix<value_type>;
    using CoordType = typename SparseMatrix::CoordType;

    typename SparseMatrix::CoordType mmparse(const std::string line) {
        std::stringstream ss{line};
        int x, y;
        value_type val;
        ss >> x >> y >> val;
        return std::make_tuple(x - 1, y - 1, val);
    }

    void parseHeader(std::string line) {
        // is this dense / sparse ?
        sparse = true;
        if (line.find("coordinate") != std::string::npos) {
        sparse = true;
        } else if (line.find("array") != std::string::npos) {
        sparse = false;
        } else {
        throw std::invalid_argument("Cannot parse header, requires either 'coordinate' or 'matrix' type");
        }

        if (line[0] =='%' && line[1] == '%') {
        if (line.find("matrix") != std::string::npos) {
            symmetric = line.find("symmetric") != std::string::npos;
        } else
            throw std::invalid_argument("Unsupported file type: " + line);
        }
    }

    bool parseHeader(bool pprint=false) {
        std::string line;
        if (!getline(*f, line))
        throw std::invalid_argument("File " + path + " is empty");

        parseHeader(line);

        // skip comments
        while (getline(*f, line) && line[0] == '%')
        continue;

        // read the dimensions
        std::stringstream ss;
        ss << line;
        ss >> nrows >> ncols;
        if (sparse)
        ss >> nnzs;
        matrix = ncols > 1;

        if (pprint) {
        std::cout << "sparse: " << sparse << std::endl;
        std::cout << "symmetric: " << symmetric << std::endl;
        std::cout << "matrix: " << matrix << std::endl;
        std::cout << "ncols: " << ncols << std::endl;
        std::cout << "nrows: " << nrows << std::endl;
        std::cout << "nnzs: " << nnzs << std::endl;
        }
    }

    std::ifstream* f;
    std::string path;

    public:

    MmReader(std::string _path) : path(_path) {
        f = new std::ifstream{_path};
        if (!f->is_open())
        throw std::invalid_argument("Could not open file path " + path);
    }

    virtual ~MmReader() {
        delete f;
    }

    std::vector<double> readVector() {
        parseHeader();
        if (matrix)
        throw std::invalid_argument("Object has > 1 columns ==> Use readMatrix");
        if (sparse)
        throw std::invalid_argument("Sparse vectors not supported");

        std::cout << "Reading vector" << std::endl;

        std::vector<std::tuple<int, double>> tpls;
        double val;

        std::vector<double> res;
        while (*f >> val )
        res.push_back(val);

        //tpls.push_back(std::make_tuple(i, val));
        //std::sort(tpls.begin(), tpls.end(),
        //[](std::tuple<int, double> x, std::tuple<int, double> y) {
        //return std::get<0>(x) < std::get<0>(y);
        //}
        //);

        //for (int i = 0; i < res.size(); i++)
        //res[i] = std::get<1>(tpls[i]);

        return res;
    }

    // Read the given file as a MM format description of a sparse matrix.
    // Returns a COO representation of the matrix.
    // NOTE:
    // - the values will be adjusted to 0 based indexing
    // - for symmetric matrices, the symmetric entries are also included
    // - Triplets will be sorted in lexicographical order of their coordinates (row, column)
    cask::sparse::SparkCooMatrix<value_type> mmreadMatrix(std::string path) {
        parseHeader();

        if (!matrix)
        throw std::invalid_argument("Matrix has only one column ==> Use readVector");

        // skip comments
        std::vector<CoordType> values;
        std::string line;
        for (int i = 0; i < nnzs; i++) {
        if (!getline(*f, line))
            throw std::invalid_argument("File has less than given nonzeros!");
        auto tpl = mmparse(line);
        values.push_back(tpl);
        auto row = std::get<0>(tpl);
        auto col = std::get<1>(tpl);
        if (symmetric && row != col)
            values.push_back(
                            std::make_tuple(
                                            std::get<1>(tpl),
                                            std::get<0>(tpl),
                                            std::get<2>(tpl)));
        }

        auto start = std::chrono::high_resolution_clock::now();
        std::sort(values.begin(), values.end(),
                [](CoordType x, CoordType y) {
                    return
                    std::get<0>(x) < std::get<0>(y) ||
                                        (std::get<0>(x) == std::get<0>(y) && std::get<1>(x) < std::get<1>(y));
                });
        dfesnippets::timing::print_clock_diff("Sorting took: ", start);

        cask::sparse::SparkCooMatrix<value_type> m(nrows, ncols);
        m.data = values;
        return m;
    }
};

}
}

#endif //CASK_IO_HPP
