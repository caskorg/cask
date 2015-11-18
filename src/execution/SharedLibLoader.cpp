#include <Spark/Execution.hpp>
#include <Spark/FileUtils.hpp>

namespace sll = spark::execution;

using namespace std;

sll::SharedLibLoader::SharedLibLoader(std::string _libPath) : libPath(_libPath) {
}

vector<string> sll::SharedLibLoader::available_libraries() {
  return spark::file_utils::child_files(libPath);
}

bool sll::SharedLibLoader::load_library(string path) {
  // XXX
  return false;
}
