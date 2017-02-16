#ifndef CASK_HPP
#define CASK_HPP

#include "../src/runtime/SparseMatrix.hpp"
#include <unordered_map>
#include <sstream>
#include <Spmv.hpp>
#include "../src/runtime/GeneratedImplSupport.hpp"
#include "../src/runtime/Cg.hpp"

namespace cask {

/** A thin wrapper around various implementation managers */
class CaskContext {

  cask::runtime::SpmvImplementationLoader spmvManager;

 public:

  void preprocess(const SymCsrMatrix& matrix) {
  }

  cask::spmv::BasicSpmv getSpmv(SymCsrMatrix& matrix) {
    // TODO should choose between the various implementtation types
    return cask::spmv::BasicSpmv(spmvManager.architectureWithParams(matrix.n));
  }

  cask::spmv::BasicSpmv getSpmv(CsrMatrix& matrix) {
    // TODO should choose between the various implementtation types
    return cask::spmv::BasicSpmv(spmvManager.architectureWithParams(matrix.n));
  }

  cask::solvers::Cg getCg(SymCsrMatrix& matrix) {

  }

};

}

#endif
