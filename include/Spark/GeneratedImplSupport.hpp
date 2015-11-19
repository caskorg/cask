#ifndef GENERATEDIMPLSUPPORT_H

#define GENERATEDIMPLSUPPORT_H

#include <vector>
#include <cstdint>

namespace spark {
  namespace runtime {

    // XXX Hell no...

    // base class for all generated implementations
    struct GeneratedImplementation {
      virtual ~GeneratedImplementation() {
      }
    };

    struct GeneratedSpmvImplementation : GeneratedImplementation {

      /* type of function pointer for Spmv_run function */
      using SpmvFunctionPtrType = void (*)(
          int64_t, int64_t, int64_t,
          const int64_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*);

      SpmvFunctionPtrType fptr;

      GeneratedSpmvImplementation(SpmvFunctionPtrType _fptr) :  fptr(_fptr) {
      }

      virtual ~GeneratedSpmvImplementation() {
      }

      // XXX also need to add wrapper for:
      //   - the underlying operations
      //   - getting parameters which allow to select the best implementation
    };

    /**
     * Loads generated implementations.
     *
     * Meant more as a base for classes generated from spark.py.
     */
    class ImplementationLoader {
      protected:
        std::vector<GeneratedImplementation*> impls;
      public:
      ImplementationLoader() {}

      virtual ~ImplementationLoader() {
        for (const auto& s : impls) {
          delete s;
        }
      }
    };

  }
}

#endif /* end of include guard: GENERATEDIMPLSUPPORT_H */
