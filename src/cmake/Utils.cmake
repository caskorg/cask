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

function (AddCaskGeneratedLibrary target name)
  set (LIB_FILE ${CMAKE_SOURCE_DIR}/build/lib-generated/libSpmv_${target}.so)
  add_custom_command(
    OUTPUT ${LIB_FILE}
    DEPENDS main ${CMAKE_SOURCE_DIR}/src/frontend/cask.py
    COMMAND python ../src/frontend/cask.py -d -t ${target} -p ../src/frontend/params.json -b ../test/test-benchmark -rb -bm best --cpp=${CMAKE_CXX_COMPILER}
    )

  add_custom_target(${name}_target DEPENDS ${LIB_FILE})
  add_library(${name} SHARED IMPORTED)
  add_dependencies(${name} ${name}_target)
  set_target_properties(${name}
    PROPERTIES
    IMPORTED_LOCATION ${LIB_FILE})
endfunction()
