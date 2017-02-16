function(AddGtestSuite name)
    AddGtestSuiteWithLib(${name} "")
endfunction()

function(AddGtestSuiteWithLib name library)
  set(testName Test${name})
  add_executable(${testName} test/${name}.cpp)
  target_link_libraries(${testName} gtest gtest_main gmock ${LibMKL_LIBRARIES} -lrt -fopenmp SparkCpuLib  ${library})
  add_test(
          NAME ${testName}
          COMMAND ${testName}
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

function(AddMaxelerSimTest binary name args)
  add_test(
          NAME test_sim_${name}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMAND ../src/frontend/simrunner ./${binary} ${args})
endfunction()

function(AddMaxelerHwTest binary name args)
  add_test(
          NAME test_hw_${name}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMAND ../src/frontend/hwrunner ./${binary} ${args})
endfunction()
