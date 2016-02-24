cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage -fno-inline --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")


# --- Project Options and Configurations
OPTION(SparseLib_DisableTesting "Disable execution of SparseLib tests" OFF)


# --- Useful functions and macros
function(AddMaxelerSimTest binary name args)
    add_test(
      NAME test_sim_${name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ../scripts/simrunner ./${binary} ${args})
endfunction()

function(AddMaxelerHwTest binary name args)
    add_test(
      NAME test_hw_${name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ../scripts/hwrunner ./${binary} ${args})
endfunction()

# ---
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

find_path(EIGEN_HEADER Eigen)

if (EIGEN_HEADER)
  message(STATUS "  --> EIGEN ${EIGEN_HEADER}")
  include_directories(${EIGEN_HEADER})
  add_definitions(-DUSEEIGEN)
  set(SparkCpu_src
    "${SparkCpu_src}"
    src/SparseLinearSolvers.cpp
    )
endif()


#
# --- Include directories
include_directories(dfe-snippets/include)
include_directories(tests)
include_directories(include)

# For each application we use:
# - a device library; this implements the low level device calls
# - an implementation library; this uses the device
#   library and adds CPU functions for pre/post-processing the data
# - an interface (header) for the implementation library


# SpMV Implementation Library
set(SpmvImplLib_src
  src/spmv/src/Spmv.cpp)
add_library(SpmvImplLib ${SpmvImplLib_src})

set(SparkCpu_src
  "${SparkCpu_src}"
  src/dse/Dse.cpp
  src/execution/SharedLibLoader.cpp
  )
add_library(SparkCpuLib ${SparkCpu_src})

add_executable(main src/main.cpp )
target_link_libraries(main
  -lboost_program_options -lboost_filesystem -lboost_system
  SparkCpuLib SpmvImplLib)

add_executable(test_architecture tests/test_architecture.cpp)
target_link_libraries(test_architecture
  -lboost_program_options
  SparkCpuLib SpmvImplLib )

# Mock implementation used for DSE
add_executable(test_spmv_dfe_mock tests/test_spmv.cpp)
target_link_libraries(test_spmv_dfe_mock
  -lboost_program_options -lboost_filesystem -lboost_system -ldl -L../lib-generated -lSpmv_dfe_mock
  SparkCpuLib SpmvImplLib)

# -- Test targets
include_directories(unit-tests)
add_executable(unit_test_spmv unit-tests/spmv_test.cpp)
target_link_libraries(unit_test_spmv boost_unit_test_framework -lgcov)
add_test(
  NAME unit_test_spmv
  COMMAND spmv_test)

# -- Maxeler tests
if (DEFINED ENV{MAXCOMPILERDIR} AND DEFINED ENV{MAXELEROSDIR})
  message(STATUS "  MAXCOMPILER --> $ENV{MAXCOMPILERDIR}")
  message(STATUS "  MAXELEROS   --> $ENV{MAXELEROSDIR}")
  link_directories($ENV{MAXCOMPILERDIR}/lib)
  link_directories($ENV{MAXELEROSDIR}/lib)

  add_executable(test_loader tests/test_library_loader.cpp)
  add_test(NAME test_loader COMMAND test_loader)
  target_link_libraries(test_loader SparkCpuLib -lboost_filesystem -lmaxeleros -ldl)


  # -- Simulation tests
  add_executable(test_spmv_sim tests/test_spmv.cpp)
  target_link_libraries(test_spmv_sim
    SparkCpuLib SparkCpuLib SpmvImplLib
    -L../lib-generated -lSpmv_sim -ldl -lboost_filesystem
    -lmaxeleros)

  # -- Hardware tests
  add_executable(test_spmv_dfe EXCLUDE_FROM_ALL tests/test_spmv.cpp)
  target_link_libraries(test_spmv_dfe
    SparkCpuLib SparkCpuLib SpmvImplLib
    -L../lib-generated -lSpmv_dfe -ldl -lboost_filesystem
    -lmaxeleros)

  if (NOT SparseLib_DisableTesting)
    enable_testing()
    # Should include dfe-snippets headers as they might be used in tests
    file(GLOB files test-matrices/*.mtx)
    foreach(infileName ${files})
      get_filename_component(matrix "${infileName}" NAME)
      AddMaxelerSimTest("test_spmv_sim" "${matrix}" "../test-matrices/${matrix}")
      AddMaxelerHwTest("test_spmv_dfe" "${matrix}" "../test-matrices/${matrix}")
    endforeach()
  endif()
endif()
