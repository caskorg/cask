#ifndef EXECUTION_H

#define EXECUTION_H

#include <string>
#include <vector>

namespace spark {
  namespace execution {
    // loads and manages shared libraries at runtime
    class SharedLibLoader {
      std::string libPath;
      // a handle to the currently loaded library, or NULL if no library is
      // loaded.
      void* handle;
      public:
      SharedLibLoader(std::string _libDirectoryPath);

      std::vector<std::string> available_libraries();

      bool load_library(std::string path);

      bool unload_library();
    };
  }
}

#endif /* end of include guard: EXECUTION_H */
