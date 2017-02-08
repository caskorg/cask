#ifndef SPAM_TESTUTILS_HPP
#define SPAM_TESTUTILS_HPP

#include <Spark/SpamSparseMatrix.hpp>
#include <Spark/Utils.hpp>

namespace spam {
namespace test {

void pretty_print(const CsrMatrix& m,
                  const std::vector<double>& rhs,
                  const std::vector<double>& exp,
                  const std::vector<double>& got)
{
  std::cout << "Matrix" << std::endl;
  m.pretty_print();
  spam::print(rhs, "Rhs = ");
  spam::print(exp, "Exp = ");
  spam::print(got, "Got = ");
}

}
}
#endif //SPAM_TESTUTILS_HPP
