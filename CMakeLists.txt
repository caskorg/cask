cmake_minimum_required (VERSION 3.1)
project (SparseBench C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/")
set(CUDA_PROPAGATE_HOST_FLAGS OFF)

add_subdirectory(src/cpu)
add_subdirectory(src/gpu)
