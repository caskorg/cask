cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage -fno-inline --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")

include(${CMAKE_SOURCE_DIR}/src/cmake/Utils.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/")
set(GENERATED_LIB_DIR "${CMAKE_SOURCE_DIR}/output/lib-generated")


if (BUILD_SIM)
  message(STATUS "Build sim: YES")
else()
  message(STATUS "Build sim: NO")
endif()

if (BUILD_HW)
  message(STATUS "Build hardware: YES")
else()
  message(STATUS "Build hardware: NO")
endif()

include(ExternalProject)
set(Eigen3_VERSION "3.2.8")
ExternalProject_Add( Eigen3
        URL "http://bitbucket.org/eigen/eigen/get/${Eigen3_VERSION}.tar.gz"
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
)

SET (BOOST_ROOT "/opt/boost/boost_1_57_0")
SET (BOOST_INCLUDEDIR "/opt/boost/boost-1.57.0/include")
SET (BOOST_LIBRARYDIR "/opt/boost/boost-1.57.0/lib")
SET (BOOST_MIN_VERSION "1.55.0")
set (Boost_NO_BOOST_CMAKE ON)

FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= 1.41) required.")
else()
  message(STATUS "Setting up BOOST")
  message(STATUS " Includes - ${Boost_INCLUDE_DIRS}")
  message(STATUS " Library  - ${Boost_LIBRARY_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif (NOT Boost_FOUND)

# ---- Project wide configuration ----
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

include_directories(Eigen3-prefix/src/Eigen3/Eigen)

find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES} -lrt -fopenmp)
  add_definitions(-DUSEMKL)
else()
  message(STATUS "Found MKL: no (set MKLROOT?)")
endif()

#
# --- Include directories
include_directories(lib/dfe-snippets/include)
include_directories(test)
include_directories(include)
include_directories(src/runtime)

# --- Software only sources used in the DSE and runtime phase
set(SparkCpu_src
        src/runtime/SparseLinearSolvers.cpp
        include/Cask.hpp
        src/runtime/SparseMatrix.hpp
        src/runtime/Spmv.cpp
        src/runtime/IO.hpp
        src/runtime/Model.hpp
        src/runtime/Utils.hpp
        src/runtime/Dse.cpp src/runtime/Cg.cpp)
add_library(SparkCpuLib ${SparkCpu_src})
add_executable(main src/main.cpp )
target_link_libraries(main -lboost_program_options -lboost_filesystem -lboost_system SparkCpuLib)

set (LIB_FILE ${CMAKE_SOURCE_DIR}/build/lib-generated/libSpmv_dfe_mock.so)
add_custom_command(
       OUTPUT ${LIB_FILE}
       DEPENDS main ${CMAKE_SOURCE_DIR}/src/frontend/cask.py
       COMMAND python ../src/frontend/cask.py -d -t dfe_mock -p ../src/frontend/params.json -b ../test/test-benchmark -rb -bm best --cpp=${CMAKE_CXX_COMPILER}
)

add_custom_target(DfeSpmvMock_target DEPENDS ${LIB_FILE})
add_library(DfeSpmvMockLib SHARED IMPORTED)
add_dependencies(DfeSpmvMockLib DfeSpmvMock_target)
set_target_properties(DfeSpmvMockLib
        PROPERTIES
        IMPORTED_LOCATION ${LIB_FILE})

add_executable(test_architecture test/test_architecture.cpp)
target_link_libraries(test_architecture
  -lboost_program_options SparkCpuLib)

# Mock implementation used for DSE
add_executable(test_spmv_dfe_mock test/test_spmv.cpp)
target_link_libraries(test_spmv_dfe_mock
  -lboost_program_options -lboost_filesystem -lboost_system -ldl DfeSpmvMockLib SparkCpuLib)

# add_executable(test_cg_mock test/test_cg.cpp)
# target_link_libraries(test_cg_mock
#   SparkCpuLib SpmvImplLib
#   -L${GENERATED_LIB_DIR} -ldl -lboost_filesystem -lSpmv_dfe_mock
#   ${LibMKL_LIBRARIES} -lrt -fopenmp)

# --- Testing infrastructure
enable_testing()
add_subdirectory(lib/gtest)
include_directories(lib/gtest/googletest/include)

# -- Maxeler tests
if (BUILD_SIM)
  if (DEFINED ENV{MAXCOMPILERDIR} AND DEFINED ENV{MAXELEROSDIR})
    message(STATUS "  MAXCOMPILER --> $ENV{MAXCOMPILERDIR}")
    message(STATUS "  MAXELEROS   --> $ENV{MAXELEROSDIR}")
    link_directories($ENV{MAXCOMPILERDIR}/lib)
    link_directories($ENV{MAXELEROSDIR}/lib)

    # -- Simulation tests
    add_executable(test_spmv_sim test/test_spmv.cpp)
    target_link_libraries(test_spmv_sim
            SparkCpuLib
            -L${GENERATED_LIB_DIR} -lSpmv_sim -ldl -lboost_filesystem
            -lmaxeleros)
  else()
    message(FATAL_ERROR "Simulation build requested, but MAXCOMPILIERDIR and MAXELEROSDIR not defined!")
  endif()
endif()

if (BUILD_HW)
  if (DEFINED ENV{MAXCOMPILERDIR} AND DEFINED ENV{MAXELEROSDIR})
    # -- Hardware tests
    add_executable(test_spmv_dfe EXCLUDE_FROM_ALL test/test_spmv.cpp)
    target_link_libraries(test_spmv_dfe
            SparkCpuLib
            -L${GENERATED_LIB_DIR} -lSpmv_dfe -ldl -lboost_filesystem
            -lmaxeleros)

    # Should include dfe-snippets headers as they might be used in tests
    file(GLOB files test/matrices/*.mtx)
    foreach(infileName ${files})
      get_filename_component(matrix "${infileName}" NAME)
      AddMaxelerSimTest("test_spmv_sim" "${matrix}" "../test/matrices/${matrix}")
      AddMaxelerHwTest("test_spmv_dfe" "${matrix}" "../test/matrices/${matrix}")
    endforeach()
  else()
    message(FATAL_ERROR "Hardware build requested, but MAXCOMPILIERDIR and MAXELEROSDIR not defined!")
  endif()
endif()

if (LibMKL_INCLUDE_DIRS)
  AddGtestSuiteWithLib(ClientTestCg DfeSpmvMockLib)
  AddGtestSuiteWithLib(ClientTestSpmv DfeSpmvMockLib)
  AddGtestSuite(LinearSolvers)
  AddGtestSuite(SparseMatrix)
  AddGtestSuite(Io)
  AddGtestSuite(MklLayer)
  AddGtestSuite(CgTest)
endif()
