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
  return 0;
}
