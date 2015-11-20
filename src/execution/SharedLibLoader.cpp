#include <Spark/Execution.hpp>
#include <Spark/FileUtils.hpp>
#include <dlfcn.h>
#include <iostream>

namespace sll = spark::execution;

using namespace std;

sll::SharedLibLoader::SharedLibLoader(std::string _libPath) : libPath(_libPath), handle(NULL) {
}

vector<string> sll::SharedLibLoader::available_libraries() {
  return spark::file_utils::child_files(libPath);
}

bool sll::SharedLibLoader::load_library(string libPath) {
  std::cout << "Loading " << libPath << std::endl;
  if (handle) {
    std::cerr << "Already loaded a library" << std::endl;
    return false;
  }
  handle = dlopen(libPath.c_str(), RTLD_LAZY);
  if (handle == NULL) {
    std::cerr << dlerror() << std::endl;
    return false;
  }
  this->libPath = libPath;
  // XXX consider what to do with the handles, might want to
  // provide an unload library method, though it's likely this library
  // will be used for the runtime of the application
  dlerror();
  return true;
}

bool sll::SharedLibLoader::unload_library() {
  if (!handle) {
    std::cerr << "Error closing library: no handle set!"
      << "call load_library() first" << std::endl;
    return false;
  }

  if (dlclose(handle) != 0) {
    std::cerr << "Error closing library" << std::endl;
    std::cerr << dlerror() << std::endl;
    return false;
  }

  handle = NULL;
  this->libPath = "";
  return true;
}
