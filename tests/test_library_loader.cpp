#include <Spark/Execution.hpp>
#include <vector>
#include <iostream>
#include <string>

using namespace spark::execution;
using namespace std;

int main() {
  SharedLibLoader loader{"../lib-generated"};
  vector<string> libs = loader.available_libraries();
  for (const string& s : libs) {
    std::cout << s << std::endl;
  }
  if (libs.size() == 0) return 1;
  string path = libs[0];
  std::cout << "Loading " << path << std::endl;
  if (loader.load_library(path)) {
    std::cout << "Success!" << std::endl;
    return 0;
  }

  std::cout << "Failed" << std::endl;
  return 1;
}
