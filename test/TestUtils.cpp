#include <Utils.hpp>
#include <gtest/gtest.h>

using namespace std;
using namespace cask::utils;

TEST(TestUtils, TestParameter) {
  Parameter<int> p{"p", 1, 1, 1};
  ASSERT_EQ(p.first().value, 1);
}

TEST(TestUtils, ChainedParameterRange) {
  std::vector<Parameter<int>> params = {
      {"numPipes", 1, 3, 1},
      {"frequency", 100, 150, 10}
  };
  ChainedParameterRange<int> cpr(params);
  cpr.start();

  // check several values from the start of the range
  ASSERT_EQ(cpr.getParam("numPipes").value, 1);
  ASSERT_EQ(cpr.getParam("frequency").value, 100);
  cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 2);
  ASSERT_EQ(cpr.getParam("frequency").value, 100);
  cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 3);
  ASSERT_EQ(cpr.getParam("frequency").value, 100);
  cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 1);
  ASSERT_EQ(cpr.getParam("frequency").value, 110);

  // resets to  first value after start()
  cpr.start();
  ASSERT_EQ(cpr.getParam("numPipes").value, 1);
  ASSERT_EQ(cpr.getParam("frequency").value, 100);

  // check everything as expected at the end of the range
  for (int i = 0; i < 15; i++)
    cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 1);
  ASSERT_EQ(cpr.getParam("frequency").value, 150);
  cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 2);
  ASSERT_EQ(cpr.getParam("frequency").value, 150);
  cpr.next();
  ASSERT_EQ(cpr.getParam("numPipes").value, 3);
  ASSERT_EQ(cpr.getParam("frequency").value, 150);
}


TEST(TestUtils, ChainedParameterRangeThreeParams) {
  std::vector<Parameter<int>> params = {
      {"numPipes", 1, 2, 1},
      {"frequency", 2, 3, 1},
      {"memory", 4, 5, 1}
  };
  ChainedParameterRange<int> cpr(params);
  cpr.start();
  for (int i = 0; i < 7; i++) {
    std::cout << cpr.getParam("numPipes").value << std::endl;
    std::cout << cpr.getParam("frequency").value << std::endl;
    std::cout << cpr.getParam("memory").value << std::endl;
    cpr.next();
  }
  std::cout << cpr.getParam("numPipes").value << std::endl;
  std::cout << cpr.getParam("frequency").value << std::endl;
  std::cout << cpr.getParam("memory").value << std::endl;
}
