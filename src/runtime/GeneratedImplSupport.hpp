#ifndef GENERATEDIMPLSUPPORT_H
#define GENERATEDIMPLSUPPORT_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <climits>
#include <functional>

/**
 * This module captures device implementation aspects:
 * - parameters (such as number of parallel pipes, memory contrllers etc.)
 * - interface (function call API for reading / writing/ runing)
 *
 * The intended use cases are:
 * 1. use a mock device interface for software simulation and design space
 *    exploration; this means we can use most of the code, including
 *    preprocesssings and cycle count computations, without having to instantiate
 *    a design.
 * 2. create a wrapper around multiple device implementations:
 *   new GeneratedSpmvImplementation(implParameters, deviceInterfaceFunctions)
 *   new GeneratedSpmvImplementation(implParameters, deviceInterfaceFunctions)
 *   This wrapper can be used to select at runtime the correct device to use
 *   based on the propoerties of the input matrix or other user input.
 */
namespace cask {
  namespace runtime {

    /* Stubs for SpMV device functions (run/read/write). Used to enable parts
     * of the flow when device libraries are not available and/or necessary. */
    inline void spmvReadMock(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        uint8_t *outstream_tocpu,
        const char* routing) { }

    inline void spmvRunMock(
        int64_t, int64_t, int64_t,
        const int64_t*, const int32_t*, const int64_t*,
        const int32_t*, const int32_t*, const int64_t*,
        const int32_t*, const int32_t*, const int64_t*) {}

    inline void spmvWriteMock(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        const uint8_t *instream_fromcpu,
        const char* routing) {}

    class GeneratedSpmvImplementation {
      //[> type of function pointers for Spmv_run/dramWrite/dramRead function <]
      using SpmvFunctionT = decltype(spmvRunMock);
      using SpmvDramWriteFunctionT = decltype(spmvWriteMock);
      using SpmvDramReadFunctionT = decltype(spmvReadMock);

      public:
      const int id, max_rows, num_pipes, cache_size, input_width, dram_reduction_enabled, num_controllers;
      std::function<SpmvFunctionT> Spmv;
      std::function<SpmvDramWriteFunctionT> write;
      std::function<SpmvDramReadFunctionT> read;
      GeneratedSpmvImplementation(
          int _id,
          SpmvFunctionT _fptr,
          SpmvDramWriteFunctionT _dramWrite,
          SpmvDramReadFunctionT _dramRead,
          int _max_rows,
          int _num_pipes,
          int _cache_size,
          int _input_width,
          int _dram_reduction_enabled,
          int _num_controllers
          ) :
        id(_id),
        Spmv(_fptr),
        write(_dramWrite),
        read(_dramRead),
        max_rows(_max_rows),
        num_pipes(_num_pipes),
        cache_size(_cache_size),
        input_width(_input_width),
        dram_reduction_enabled(_dram_reduction_enabled),
        num_controllers(_num_controllers)
      {}

      bool operator==(const GeneratedSpmvImplementation& other) const {
        return
          max_rows == other.max_rows &&
          num_pipes == other.num_pipes &&
          cache_size == other.cache_size &&
          input_width == other.input_width &&
          dram_reduction_enabled == other.dram_reduction_enabled &&
          num_controllers == other.num_controllers;
      }
    };


    // provide interface for SpmvImplementation loader
    class SpmvImplementationLoader {

      std::vector<GeneratedSpmvImplementation*> impls;

      public:
      SpmvImplementationLoader();

      /**
       * Load the generated spmv implementation which supports the given number
       * of rows. If more exist, picks the one with smallest maxRows.
       */
      GeneratedSpmvImplementation* architectureWithParams(int maxRows) {
        GeneratedSpmvImplementation* bestArch = nullptr;
        for (const auto& a : this->impls) {
          if (a->max_rows >= maxRows && (!bestArch || a->max_rows < bestArch->max_rows)) {
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
