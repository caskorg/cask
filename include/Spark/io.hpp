#ifndef IO_HPP_QPBO8W0X
#define IO_HPP_QPBO8W0X

#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <stdexcept>
#include <fstream>

#include <Spark/SparseMatrix.hpp>

namespace spark {
  namespace io {

    template<typename value_type>
      class MmReader {

        bool sparse = false, symmetric = false, matrix = false;
        int ncols, nrows, nnzs = -1;

        using SparseMatrix = spark::sparse::SparkCooMatrix<value_type>;
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
                std::cout << "Is symmetric" << std::endl;
              } else
                throw std::invalid_argument("Unsupported file type: " + line);
            }
            std::cout << "Header " << line  << std::endl;
        }

        bool parseHeader() {
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

          std::cout << "sparse: " << sparse << std::endl;
          std::cout << "symmetric: " << symmetric << std::endl;
          std::cout << "matrix: " << matrix << std::endl;
          std::cout << "ncols: " << ncols << std::endl;
          std::cout << "nrows: " << nrows << std::endl;
          std::cout << "nnzs: " << nnzs << std::endl;
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
        spark::sparse::SparkCooMatrix<value_type> mmreadMatrix(std::string path) {
          parseHeader();

          if (!matrix)
            throw std::invalid_argument("Matrix has only one column ==> Use readVector");

          // skip comments
          std::vector<CoordType> values;
          std::string line;
          while (getline(*f, line)) {
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

          std::sort(values.begin(), values.end(),
              [](CoordType x, CoordType y) {
              return
              std::get<0>(x) < std::get<0>(y) ||
              (std::get<0>(x) == std::get<0>(y) && std::get<1>(x) < std::get<1>(y));
              });

          spark::sparse::SparkCooMatrix<value_type> m(nrows, ncols);
          m.data = values;
          return m;
        }
      };
  }
}



#endif /* end of include guard: IO_HPP_QPBO8W0X */
