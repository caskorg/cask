#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE spmv_test
#include <boost/test/unit_test.hpp>

#include <Spark/Spmv.hpp>

BOOST_AUTO_TEST_CASE(logic_resource_usage_test)
{
  spark::model::LogicResourceUsage ru{1, 1, 1, 1};
  BOOST_CHECK_EQUAL(ru.luts, 1);
}
