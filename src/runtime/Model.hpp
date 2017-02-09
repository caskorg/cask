#ifndef MODEL_H
#define MODEL_H

#include <sstream>

namespace spark {
  namespace model {

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
