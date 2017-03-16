#ifndef MODEL_H
#define MODEL_H

#include <sstream>
#include <iostream>

namespace cask {
  namespace model {

    /** Abstract representation of the device we are implementing for. */
    class DeviceModel {
      public:
        virtual int entriesPerBram(int bitwidth) const = 0;
        virtual std::string getId() const = 0;
    };

    /** Abstract representation of a Max4 board */
    class Max4Model : public DeviceModel {
      public:
        int entriesPerBram(int bitwidth) const override {
          if (bitwidth == 64) {
            // On SV we need two BRAMs to store 512 entries of 64 bits wide, so
            // on average we store 256 values per BRAM
            return 256;
          }
          throw std::invalid_argument("Only bitwidth == 64 supported for now");
        }
        std::string getId() const override {
          return "Max4";
        }
    };

    inline std::ostream& operator<<(std::ostream& out, const DeviceModel& dm) {
      out << dm.getId();
      return out;
    }

    /* Note, that this device model is no longer supported  as it is rather
     * old. Most importantly, Maxeler don't support multiple memory channels
     * on this device, so it makes maintaining the architecture painful. Some
     * indicative numbers from past implementations are provided below, but
     * should be taken as a very rough guesstimate, since the architecture
     * would have changed since then.
     */
    class Max3Model : public DeviceModel {
      public:
        virtual int entriesPerBram(int bitwidth) const override {
          if (bitwidth == 64) {
            return 512;
          }
          throw std::invalid_argument("Only bitwidth == 64 supported for now");
        }

        std::string getId() const override {
          return "Max3";
        }

       // NOTE obsolete resource usage numbers for the basic SpMV architecture, tread carefully!
       //  LogicResourceUsage interPartitionReductionKernel(2768,1505, maxRows / virtex6EntriesPerBram, 0);
       //  LogicResourceUsage paddingKernel{400, 500, 0, 0};
       //  LogicResourceUsage spmvKernelPerInput{1466, 2060, cacheSize / virtex6EntriesPerBram, 10}; // includes cache
       //  LogicResourceUsage sm{800, 500, 0, 0};
       //  LogicResourceUsage spmvPerPipe =
       //    interPartitionReductionKernel +
       //    paddingKernel +
       //    spmvKernelPerInput * inputWidth +
       //    sm;
       //  LogicResourceUsage memoryPerPipe{3922, 8393, 160, 0};
       //  LogicResourceUsage memory{24000, 32000, 0, 0};
       //  LogicResourceUsage designUsage = (spmvPerPipe + memoryPerPipe) * numPipes + memory;
       //  double memoryBandwidth =(double)inputWidth * numPipes * getFrequency() * 12.0 / 1E9;
       //  ImplementationParameters ip{designUsage, memoryBandwidth};
    };

    class LogicResourceUsage {
      public:
        int luts, ffs, brams, dsps;

        LogicResourceUsage() : luts(0), ffs(0), dsps(0), brams(0) {}

        LogicResourceUsage(int _l, int _f, int _b, int _d) :
          luts(_l), ffs(_f), brams(_b), dsps(_d) {}

        LogicResourceUsage(const LogicResourceUsage& ru) {
          luts = ru.luts;
          ffs = ru.ffs;
          dsps = ru.dsps;
          brams = ru.brams;
        }

        std::string to_string() {
          std::stringstream s;
          s << luts << " " << ffs << " " << " " << dsps << " " << brams;
          return s.str();
        }

        const LogicResourceUsage operator+(const LogicResourceUsage& ru) const {
          return LogicResourceUsage(luts + ru.luts, ffs + ru.ffs, brams + ru.brams, dsps + ru.dsps);
        }

        const LogicResourceUsage operator*(int x) const {
          return LogicResourceUsage(x * luts,  x * ffs, x * brams, x * dsps);
        }

        const LogicResourceUsage operator*(double x) const {
          return LogicResourceUsage(x * (double)luts,  x * (double)ffs, x * (double)brams, x * (double)dsps);
        }

        const bool operator<(const LogicResourceUsage& ru) const {
          return
            luts < ru.luts &&
            ffs < ru.ffs &&
            dsps < ru.dsps &&
            brams < ru.brams;
        }
    };

    // models the implementation parameters which are constrained (e.g. by
    // physical properties, such as lut usage, or by the compiler, such as the
    // number of streams)
    class ImplementationParameters {
      public:

        LogicResourceUsage ru;

        //  params belows not being used currently
        double memoryBandwidth; // <-- would be nice to have this
        //int streams;
        //int clockFrequency;
        //int memoryClockFrequency;

        ImplementationParameters(const LogicResourceUsage& _ru, double _memoryBandwidth) : ru(_ru), memoryBandwidth(_memoryBandwidth) {}

        std::string to_string() {
          std::stringstream s;
          s << ru.to_string() << " " << memoryBandwidth;
          return s.str();
        }

        bool operator<(const ImplementationParameters& other) const {
          return
            ru < other.ru &&
            memoryBandwidth < other.memoryBandwidth;
          //stream < other.streams &&
          //clockFrequency < other.clockFrequency &&
          //memoryClockFrequency < other.memoryClockFrequency;
        }
    };


  }
}
#endif /* end of include guard: MODEL_H */
