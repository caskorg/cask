#ifndef GENERATEDIMPLSUPPORT_H

#define GENERATEDIMPLSUPPORT_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <climits>

namespace spark {
  namespace runtime {

    // XXX Hell no...

    // base class for all generated implementations
    struct GeneratedImplementation {
      virtual ~GeneratedImplementation() {
      }
    };

    class GeneratedSpmvImplementation : public GeneratedImplementation {

      /* type of function pointers for Spmv_run/dramWrite/dramRead function */
      using SpmvFunctionPtr = void (*)(
          int64_t, int64_t, int64_t,
          const int64_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*);

      using SpmvDramWriteFunctionPtr = void (*)(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          const uint8_t *instream_fromcpu);

      using SpmvDramReadFunctionPtr = void (*)(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          uint8_t *instream_fromcpu);

      const SpmvFunctionPtr runSpmv;
      const SpmvDramWriteFunctionPtr dramWrite;
      const SpmvDramReadFunctionPtr dramRead;

      const int max_rows, num_pipes, cache_size, input_width, dram_reduction_enabled;

      public:

      GeneratedSpmvImplementation(
          SpmvFunctionPtr _fptr,
          SpmvDramWriteFunctionPtr _dramWrite,
          SpmvDramReadFunctionPtr _dramRead,
          int _max_rows,
          int _num_pipes,
          int _cache_size,
          int _input_width,
          int _dram_reduction_enabled
          ) :
        runSpmv(_fptr),
        dramWrite(_dramWrite),
        dramRead(_dramRead),
        max_rows(_max_rows),
        num_pipes(_num_pipes),
        cache_size(_cache_size),
        input_width(_input_width),
        dram_reduction_enabled(_dram_reduction_enabled)
      {}

      virtual ~GeneratedSpmvImplementation() {
      }

      int maxRows() {
        return max_rows;
      }

      int numPipes() {
        return num_pipes;
      }

      int cacheSize() {
        return cache_size;
      }

      int inputWidth() {
        return input_width;
      }

      bool getDramReductionEnabled() {
        return dram_reduction_enabled == 1;
      }

      void Spmv(
          int64_t param_nIterations,
          int64_t param_nPartitions,
          int64_t param_vectorLoadCycles,
          const int64_t *param_colPtrStartAddresses,
          const int32_t *param_colptrSizes,
          const int64_t *param_indptrValuesAddresses,
          const int32_t *param_indptrValuesSizes,
          const int32_t *param_nrows,
          const int64_t *param_outStartAddresses,
          const int32_t *param_reductionCycles,
          const int32_t *param_totalCycles,
          const int64_t *param_vStartAddresses) {

        std::cout << "Running on architecture with " << std::endl;
        std::cout << "   maxRows = " << max_rows << std::endl;

        (*runSpmv)(
          param_nIterations,
          param_nPartitions,
          param_vectorLoadCycles,
          param_colPtrStartAddresses,
          param_colptrSizes,
          param_indptrValuesAddresses,
          param_indptrValuesSizes,
          param_nrows,
          param_outStartAddresses,
          param_reductionCycles,
          param_totalCycles,
          param_vStartAddresses);
      }

      void write(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          const uint8_t *instream_fromcpu) {
        (*dramWrite)(
          param_size_bytes,
          param_start_bytes,
          instream_fromcpu);
      }


      void read(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          uint8_t *outstream_tocpu) {
        (*dramRead)(
          param_size_bytes,
          param_start_bytes,
          outstream_tocpu);
      }

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

    // provide interface for SpmvImplementation loader
    class SpmvImplementationLoader : public ImplementationLoader {
      public:
      SpmvImplementationLoader();

      /**
       * Load the generated spmv implementation which supports the given number
       * of rows. If more exists, picks the one with smallest maxRows.
       */
      GeneratedSpmvImplementation* architectureWithParams(int maxRows) {
        GeneratedSpmvImplementation* bestArch = nullptr;
        for (const auto& a : this->impls) {
          GeneratedSpmvImplementation* simpl = static_cast<GeneratedSpmvImplementation*>(a);
          if (simpl->maxRows() > maxRows &&
              (!bestArch || simpl->maxRows() < bestArch->maxRows())) {
            bestArch = simpl;
          }
        }
        return  bestArch;
      }

    };

  }
}

#endif /* end of include guard: GENERATEDIMPLSUPPORT_H */
