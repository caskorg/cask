cmake_minimum_required(VERSION 2.6)

OPTION(SparseLib_DisableTesting "Disable execution of SparseLib tests" OFF)

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
  set(Stream_Grid_sources
    "${Stream_Grid_sources}"
    src/SparseLinearSolvers.cpp
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

  # Set all Maxeler related files
  #set(Stream_Grid_sources
    #"${Stream_Grid_sources}"
    #OneStepDfeSolver.cpp)
  include_directories(
    dfe/StreamGrid/include/)

  set(
    StreamGridDfeBuild
    "${CMAKE_CURRENT_LIST_DIR}/build"
    )

  set (StreamGridDfeSources
    include/Spark/StreamGridDfe.hpp
    src/StreamGridKernel.maxj
    src/StreamGridManager.maxj)

  # XXX Should use a find_library() to check for these library instead of including them directly
  set(dfelibraries
    -l. -lstreamgrid_sim)

  add_custom_target(
    simlib
    WORKING_DIRECTORY .
    COMMAND make -C ${StreamGridDfeBuild} simlib
    COMMAND cp ${StreamGridDfeBuild}/lib* ${CMAKE_BINARY_DIR}
    DEPENDS ${StreamGridDfeSources})

  add_custom_target(
    dfe_sim_distclean
    COMMAND make -C ${StreamGridDfeBuild} distclean)

endif()

add_library(StreamGridLib ${Stream_Grid_sources})

add_dependencies(StreamGridLib simlib)

if (NOT SparseLib_DisableTesting)
  message(STATUS "Tests not disabled")
  enable_testing()
  # Should include dfe-snippets headers as they might be used in tests
  include_directories(dfe-snippets/include)
  add_executable(
    test_solver1
    tests/test_solver1.cpp)
  target_link_libraries(
    test_solver1
    StreamGridLib
    -lumfpack -lsuitesparseconfig -lopenblas -lcholmod -lcolamd -lrt -lklu -lbtf -lamd -lpthread)
  add_test(
    NAME test_solver_small
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ../scripts/simrunner ./test_solver1)
endif()
