#ifndef EXECUTION_H

#define EXECUTION_H

#include <string>
#include <vector>

namespace spark {
  namespace execution {
    // loads and manages shared libraries at runtime
    class SharedLibLoader {
      std::string libPath;
      public:
      SharedLibLoader(std::string _libDirectoryPath);

      std::vector<std::string> available_libraries();

      bool load_library(std::string path);

    };
  }
}

#endif /* end of include guard: EXECUTION_H */
