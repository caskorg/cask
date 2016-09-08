cmake_minimum_required (VERSION 3.5)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 14)

project (SparseBench C CXX)

add_subdirectory(src/cpu)
