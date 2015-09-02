cmake_minimum_required(VERSION 2.6)

OPTION(SparseLib_DisableTesting "Disable execution of SparseLib tests" OFF)

include_directories(include)

# ---- Useful functions and macros

# Adds a maxeler implementation
function(AddMaxelerImpl path stem target)
  set(DfeBuild "${CMAKE_CURRENT_LIST_DIR}/src/${path}/build")
  set(Src
    include/Spark/SparseLinearSolvers.hpp
    src/${path}/src/${stem}Kernel.maxj
    src/${path}/src/${stem}Manager.maxj
    src/${path}/src/${stem}.cpp)
  add_custom_target(
    ${target}
    WORKING_DIRECTORY .
    COMMAND make -C ${DfeBuild} simlib
    COMMAND cp ${DfeBuild}/lib* ${CMAKE_BINARY_DIR}
    DEPENDS ${Src})
  add_custom_target(
    ${target}_dfe
    WORKING_DIRECTORY .
    COMMAND make -C ${DfeBuild} dfelib
    COMMAND cp ${DfeBuild}/lib* ${CMAKE_BINARY_DIR}
    DEPENDS ${Src})
endfunction()

function(AddMaxelerImplTest name libtarget lib)
    add_executable(
      test_${name}
      tests/test_${name}.cpp)
    add_dependencies(test_${name} ${libtarget})
    target_link_libraries(
      test_${name}
      -L. -l${lib}
      SparkCpuLib
      -lumfpack -lsuitesparseconfig -lopenblas -lcholmod
      -lcolamd -lrt -lklu -lbtf -lamd -lpthread)
    add_test(
      NAME test_${name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ../scripts/simrunner ./test_${name})
endfunction()

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
    #AddMaxelerImplTest("spmv" spmvlib "Spmv_sim")

    AddMaxelerImplTest("bicg" simbicglib "BiConjugateGradient_sim")
    add_dependencies(test_bicg smpvlib)
    target_link_libraries(test_bicg -lSpmv_sim)

  endif()

endif()

