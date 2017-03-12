#ifndef GENERATEDIMPLSUPPORT_H

#define GENERATEDIMPLSUPPORT_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <climits>

namespace cask {
  namespace runtime {

    class GeneratedSpmvImplementation {

      /* type of function pointers for Spmv_run/dramWrite/dramRead function */
      using SpmvFunctionPtr = void (*)(
          int64_t, int64_t, int64_t,
          const int64_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*,
          const int32_t*, const int32_t*, const int64_t*);

      using SpmvDramWriteFunctionPtr = void (*)(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        const uint8_t *instream_fromcpu,
        const char* routing);

      using SpmvDramReadFunctionPtr = void (*)(
          const int64_t *param_size_bytes_cpu,
          const int64_t *param_size_bytes_memory_ctl,
          const int64_t *param_start_bytes_memory_ctl,
          uint8_t *outstream_tocpu0,
          uint8_t *outstream_tocpu1);

      protected:
      const int id;
      const SpmvFunctionPtr runSpmv;
      const SpmvDramWriteFunctionPtr dramWrite;
      const SpmvDramReadFunctionPtr dramRead;

      const int max_rows, num_pipes, cache_size, input_width, dram_reduction_enabled;

      public:

      GeneratedSpmvImplementation(
          int _id,
          SpmvFunctionPtr _fptr,
          SpmvDramWriteFunctionPtr _dramWrite,
          SpmvDramReadFunctionPtr _dramRead,
          int _max_rows,
          int _num_pipes,
          int _cache_size,
          int _input_width,
          int _dram_reduction_enabled
          ) :
        id(_id),
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

      virtual void Spmv(
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

        // XXX this is probably not the best place to print stuff
        std::cout << "Config ArchitectureId " << this->id << std::endl;

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

      virtual void write(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        const uint8_t *instream_fromcpu,
        const char* routing) {
        (*dramWrite)(param_size_bytes_cpu,
                     param_size_bytes_memory_ctl,
                     param_start_bytes_memory_ctl,
                     instream_fromcpu,
                     routing);
      }

      virtual void read(
          const int64_t *param_size_bytes_cpu,
          const int64_t *param_size_bytes_memory_ctl,
          const int64_t *param_start_bytes_memory_ctl,
          uint8_t *outstream_tocpu0,
          uint8_t *outstream_tocpu1) {
        (*dramRead)(
          param_size_bytes_cpu,
          param_size_bytes_memory_ctl,
          param_start_bytes_memory_ctl,
          outstream_tocpu0,
          outstream_tocpu1);
      }

    };

    class GeneratedSpmvImplementationMock : public GeneratedSpmvImplementation {

      public:

      GeneratedSpmvImplementationMock (
          int _max_rows,
          int _num_pipes,
          int _cache_size,
          int _input_width,
          int _dram_reduction_enabled
          ) :
        GeneratedSpmvImplementation(
            -1, nullptr, nullptr, nullptr,
            _max_rows, _num_pipes, _cache_size,
            _input_width, _dram_reduction_enabled) {}

      virtual ~GeneratedSpmvImplementationMock() {
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
        std::cout << "Running mock SPMV implemetation" << std::endl;
      }

      void write(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          const uint8_t *instream_fromcpu) {}

      void read(
          int64_t param_size_bytes,
          int64_t param_start_bytes,
          uint8_t *outstream_tocpu) {}

    };

    // provide interface for SpmvImplementation loader
    class SpmvImplementationLoader {

      std::vector<GeneratedSpmvImplementation*> impls;

      public:
      SpmvImplementationLoader();

      /**
       * Load the generated spmv implementation which supports the given number
       * of rows. If more exists, picks the one with smallest maxRows.
       */
      GeneratedSpmvImplementation* architectureWithParams(int maxRows) {
        GeneratedSpmvImplementation* bestArch = nullptr;
        for (const auto& a : this->impls) {
          std::cout << maxRows << std::endl;
          std::cout << a->maxRows() << std::endl;
          if (a->maxRows() >= maxRows &&
              (!bestArch || a->maxRows() < bestArch->maxRows())) {
            bestArch = a;
          }
        }
        return  bestArch;
      }

      GeneratedSpmvImplementation* architectureWithId(int id) {
        return static_cast<GeneratedSpmvImplementation*>(this->impls.at(id));
      }

    };

  }
}

#endif /* end of include guard: GENERATEDIMPLSUPPORT_H */
