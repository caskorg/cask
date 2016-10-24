cmake_minimum_required (VERSION 3.1)
project (spam C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Xhost")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake/")

include_directories(include)


find_package(Eigen REQUIRED)

add_subdirectory(lib/gtest)
enable_testing()


add_library(common SHARED src/mmio.c src/mmio.h src/common.c src/common.h)
include_directories(src/)

#
# if (LibEIGEN_INCLUDE_DIRS)
#   message(STATUS "Found EIGEN --> ${LibEIGEN_INCLUDE_DIRS}")
#   include_directories(lib/)
#   include_directories(${LibEIGEN_INCLUDE_DIRS})
#   add_executable(BicgStabEigen BicgStabEigen.cpp EigenUtils.hpp BenchmarkUtils.hpp)
#   add_executable(CgEigen CgEigen.cpp EigenUtils.hpp BenchmarkUtils.hpp)
#   target_link_libraries(CgEigen mmio)
#   target_link_libraries(BicgStabEigen mmio)
# endif()
#
find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  # include_directories(lib)
  # // set(common_sources
  # //   lib/mmio.c
  # //   lib/common.c
  # //   )
  # // add_library(common ${common_sources})

  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES} common -lrt -fopenmp)
endif()


add_executable(LinearSolvers test/LinearSolvers.cpp)
target_link_libraries(LinearSolvers gtest gtest_main gmock ${LibMKL_LIBRARIES} -lrt -fopenmp common)
add_test(LinearSolvers LinearSolvers)
