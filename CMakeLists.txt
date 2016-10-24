cmake_minimum_required (VERSION 3.1)
project (spam C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Xhost")
endif()

find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES} common -lrt -fopenmp)
else()
  message(FATAL_ERROR "MKL is required for current version. \
                       Please set MKLROOT to the top level directory of your MKL path")
endif()

include_directories(include)
include_directories(src/)

add_library(common SHARED src/mmio.c src/mmio.h src/common.c src/common.h)

add_subdirectory(lib/gtest)

enable_testing()

add_executable(LinearSolvers test/LinearSolvers.cpp)
target_link_libraries(LinearSolvers gtest gtest_main gmock ${LibMKL_LIBRARIES} -lrt -fopenmp common)

set(TEST_MATRICES tinysym tiny)
foreach(matrix ${TEST_MATRICES})
  set(mat -mat test/systems/${matrix}.mtx)
  set(lhs -lhs test/systems/${matrix}_sol.mtx)
  set(rhs -rhs test/systems/${matrix}_b.mtx)
  add_test(
    NAME "LinearSol_${matrix}"
    COMMAND LinearSolvers ${mat} ${rhs} ${lhs}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endforeach()
