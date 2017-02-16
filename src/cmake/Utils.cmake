function(AddGtestSuite name)
  set(testName Test${name})
  add_executable(${testName} test/${name}.cpp)
  target_link_libraries(${testName} gtest gtest_main gmock ${LibMKL_LIBRARIES} -lrt -fopenmp)
  add_test(
          NAME ${testName}
          COMMAND ${testName}
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

function(AddGtestSuiteWithCask name)
  set(testName Test${name})
  add_executable(${testName} test/${name}.cpp)
  target_link_libraries(${testName} gtest gtest_main gmock ${LibMKL_LIBRARIES} -lrt -fopenmp
          SparkCpuLib SpmvImplLib  ${DfeSpmvMockLib})
  add_test(
          NAME ${testName}
          COMMAND ${testName}
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()
