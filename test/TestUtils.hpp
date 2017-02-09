#ifndef SPAM_TESTUTILS_HPP
#define SPAM_TESTUTILS_HPP

#include <SparseMatrix.hpp>
#include <Utils.hpp>

namespace cask {
namespace test {

void pretty_print(const CsrMatrix& m,
                  const std::vector<double>& rhs,
                  const std::vector<double>& exp,
                  const std::vector<double>& got)
{
  std::cout << "Matrix" << std::endl;
  m.pretty_print();
  cask::utils::print(rhs, "Rhs = ");
  cask::utils::print(exp, "Exp = ");
  cask::utils::print(got, "Got = ");
}

}
}
#endif //SPAM_TESTUTILS_HPP
