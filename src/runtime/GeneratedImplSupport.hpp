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
 *
 * 1. a mock device interface can be used for software simulation and
 * exploration purposes; this means we can use most of the code, including
 * preprocesssings and cycle count computations, without having to create an
 * actual design, neither simulation nor hardware.
 *
 * 2. a wrapper for  several device implementations can be created as follows
 *
 *   new GeneratedSpmvImplemenation<RunFunction1, ReadFunction1, WriteFunction1>(implementationParameters)
 *   new GeneratedSpmvImplemenation<RunFunction2, ReadFunction2, WriteFunction2>(implementationParameters)
 *
 * This enables us to programmatically create a wrapper around several API
 * calls generated by the MaxCompiler. This wrapper can be used to select at
 * runtime the correct device to use based on the propoerties of the input
 * matrix or other user input.
 */
namespace cask {
  namespace runtime {

    struct ImplementationParameters {
      const int id, max_rows, num_pipes, cache_size, input_width, num_controllers;
      bool dram_reduction_enabled;
      ImplementationParameters(
          int _id,
          int _max_rows,
          int _num_pipes,
          int _cache_size,
          int _input_width,
          int _dram_reduction_enabled,
          int _num_controllers) :
        id(_id),
        max_rows(_max_rows),
        num_pipes(_num_pipes),
        cache_size(_cache_size),
        input_width(_input_width),
        dram_reduction_enabled(_dram_reduction_enabled),
        num_controllers(_num_controllers) {}

      bool operator==(const ImplementationParameters& other) const {
        return max_rows == other.max_rows &&
            num_pipes == other.num_pipes &&
            cache_size == other.cache_size &&
            input_width == other.input_width &&
            dram_reduction_enabled == other.dram_reduction_enabled &&
            num_controllers == other.num_controllers;
      }
    };

    // stub interfaces for Spmv run / write /read calls; generated bitstream
    // APIs should provide this interface
    inline void Spmv(
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
        const int64_t *param_vStartAddresses) { }

    inline void write(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        const uint8_t *instream_fromcpu,
        const char* routing) { }

    inline void read(
        const int64_t param_size_bytes_cpu,
        const int64_t *param_size_bytes_memory_ctl,
        const int64_t *param_start_bytes_memory_ctl,
        uint8_t *outstream_tocpu,
        const char* routing
        ) { }

    /* Generic wrapper for thre function device interfaces */
    template<typename RunFunction, typename ReadFunction, typename WriteFunction>
    struct DeviceInterface {
      std::function<RunFunction> run;
      std::function<ReadFunction> read;
      std::function<WriteFunction> write;
      DeviceInterface(RunFunction _run, ReadFunction _read, WriteFunction _write)
        : run(_run), read(_read), write(_write) { }
    };

    /* Interface provided by the Spmv implementation*/
    using SpmvDeviceInterfaceT = DeviceInterface<decltype(Spmv), decltype(read), decltype(write)>;
    /**
     * A stub interface; use this when the actual device routines should not be
     * called, such as for the design space exploration process
     */
    const DeviceInterface<decltype(Spmv), decltype(read), decltype(write)> stubDeviceInterface(Spmv, read, write);

    struct GeneratedSpmvImplementation {
      const ImplementationParameters params;
      const SpmvDeviceInterfaceT& deviceInterface;
      GeneratedSpmvImplementation(
          const ImplementationParameters& _params,
          const SpmvDeviceInterfaceT& _deviceInterface)
        : params(_params), deviceInterface(_deviceInterface) {}
      bool operator==(const GeneratedSpmvImplementation& other) const {
        return params == other.params;
      }
    };

    // provide interface for SpmvImplementation loader
    class SpmvImplementationLoader {
      std::vector<GeneratedSpmvImplementation> impls;
      public:

      SpmvImplementationLoader();

      /**
       * Load the generated spmv implementation which supports the given number
       * of rows. If more exists, picks the one with smallest maxRows.
       */
      GeneratedSpmvImplementation architectureWithParams(int maxRows) {
        const GeneratedSpmvImplementation* bestArch = nullptr;
        for (const auto& a : this->impls) {
          std::cout << maxRows << std::endl;
          std::cout << a.params.max_rows << std::endl;
          if (a.params.max_rows >= maxRows &&
              (!bestArch || a.params.max_rows < bestArch->params.max_rows)) {
            bestArch = &a;
          }
        }
        return  *bestArch;
      }

      GeneratedSpmvImplementation architectureWithId(int id) {
        return impls.at(id);
      }

    };

  }
}

#endif /* end of include guard: GENERATEDIMPLSUPPORT_H */
