cmake_minimum_required(VERSION 2.6)

# ---- Useful functions and macros
include(scripts/CMakeFunctions.txt)

OPTION(SparseLib_DisableTesting "Disable execution of SparseLib tests" OFF)

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

include_directories(include)

# ---- Project wide configuration ----
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

find_library(KLU_LIB klu)
find_path(
  EIGEN_HEADER
  Eigen)

message(STATUS "Found Implementations:")

if (UMFPACK_LIB)
  add_definitions(-DUSEUMFPACK)
  message(STATUS "  --> UMFPACK ${UMFPACK_LIB}")
endif()

if (EIGEN_HEADER)
  message(STATUS "  --> EIGEN ${EIGEN_HEADER}")
  add_definitions(-DUSEEIGEN)
  set(SparkCpu_src
    "${SparkCpu_src}"
    src/SparseLinearSolvers.cpp
    src/dse/Spark.cpp
    )
endif()

if (KLU_LIB)
  add_definitions(-DUSEKLU)
  message(STATUS "  --> KLU ${KLU_LIB}")
  #set(Stream_Grid_sources
    #"${Stream_Grid_sources}"
    #OneStepKluSolver.cpp
  #)
endif()

include_directories(include)
add_library(SparkCpuLib ${SparkCpu_src})

file(GLOB files test-matrices/*.mtx)

if (DEFINED ENV{MAXCOMPILERDIR} AND DEFINED ENV{MAXELEROSDIR})
  set(DFE_LIB true)
  add_definitions(-DUSEDFE)
  message(STATUS "  --> DFE")
  message(STATUS "         MAXCOMPILER --> $ENV{MAXCOMPILERDIR}")
  message(STATUS "         MAXELEROS   --> $ENV{MAXELEROSDIR}")
  include_directories ($ENV{MAXCOMPILERDIR}/include)
  include_directories ($ENV{MAXCOMPILERDIR}/include/slic)
  include_directories ($ENV{MAXELEROSDIR}/include)
  link_directories($ENV{MAXCOMPILERDIR}/lib)
  link_directories($ENV{MAXELEROSDIR}/lib)

  # --- Configuration for Maxeler libs
  AddMaxelerImpl("cg" "ConjugateGradient" "simcglib")
  AddMaxelerImpl("bicg" "BiConjugateGradient" "simbicglib")
  AddMaxelerImpl("spmv" "Spmv" "spmvlib")

  add_custom_target(
    distclean
    COMMAND make -C${CMAKE_CURRENT_LIST_DIR}/src/cg/build distclean
    COMMAND make -C${CMAKE_CURRENT_LIST_DIR}/src/bicg/build distclean
    )

  if (NOT SparseLib_DisableTesting)
    enable_testing()

    # Should include dfe-snippets headers as they might be used in tests
    include_directories(dfe-snippets/include)
    include_directories(tests/)

    #AddMaxelerImplTest("cg" simcglib "ConjugateGradient_sim")
    AddMaxelerImplTest("spmv" spmvlib "Spmv_sim" "")

    foreach(infileName ${files})
      get_filename_component(bleah "${infileName}" NAME)
      AddMaxelerImplTestRun("test_spmv" "${bleah}" "../test-matrices/${bleah}")
      AddSpmvTestRunHw("${bleah}")
    endforeach()

    AddMaxelerImplTest("bicg" simbicglib "BiConjugateGradient_sim" "")
    AddMaxelerImplTestRun("test_bicg" "bicg" "")
    add_dependencies(test_bicg smpvlib)
    target_link_libraries(test_bicg -lSpmv_sim)

    AddMaxelerImplTest("architecture" spmvlib "Spmv_sim" "")
    AddMaxelerImplTestRun("test_architecture" "spmv_architecture_test_small" "--filePath ../test-matrices/TSOPF_RS_b39_c7.mtx")

    AddMaxelerImplTestHw("spmv" spmvlib_dfe "Spmv_dfe" "")
    AddMaxelerImplTestRunHw("test_spmv_hw" "test_spmv_hw_dense_8" "../test-matrices/test_dense_8.mtx")

    include_directories(unit-tests)
    add_executable(spmv_test unit-tests/spmv_test.cpp)
    target_link_libraries(spmv_test boost_unit_test_framework)
    add_test(
      NAME unit_test_spmv
      COMMAND spmv_test)

    add_executable(test_dse tests/test_dse.cpp )
    target_link_libraries(test_dse SparkCpuLib)
    add_test(
      NAME test_dse
      COMMAND test_dse)

  endif()
else()

  include_directories(dfe-snippets/include)

  message(STATUS "Maxeler not found")
  add_definitions("-DCPUONLY")
  add_executable(test_architecture tests/test_architecture.cpp src/spmv/src/Spmv.cpp)
  target_link_libraries(
    test_architecture
    -lboost_program_options)
  include_directories(tests)
  add_executable(test_spmv tests/test_spmv.cpp src/spmv/src/Spmv.cpp src/SparseLinearSolvers.cpp)
endif()

add_executable(main src/main.cpp)
target_link_libraries(main -lboost_program_options -lboost_filesystem -lboost_system SparkCpuLib)
