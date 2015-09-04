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

function(AddMaxelerImplTest name libtarget lib args)
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
endfunction()

function(AddMaxelerImplTestRun binary name args)
    add_test(
      NAME test_${name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ../scripts/simrunner ./${binary} ${args})
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
    AddMaxelerImplTest("spmv" spmvlib "Spmv_sim" "")

    AddMaxelerImplTestRun("test_spmv" "spmv_dense128" "../test-matrices/test_dense_128.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_small" "../test-matrices/test_small.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_dense_128" "../test-matrices/test_dense_128.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_dense_8" "../test-matrices/test_dense_8.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_empty_last_rows_small" "../test-matrices/test_empty_last_rows_small.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_large_dense" "../test-matrices/test_large_dense.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_longer_row" "../test-matrices/test_longer_row.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_long_row" "../test-matrices/test_long_row.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_no_empty_rows" "../test-matrices/test_no_empty_rows.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_non_multiple" "../test-matrices/test_non_multiple.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_one_row" "../test-matrices/test_one_row.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_one_value_one_index_per_row" "../test-matrices/test_one_value_one_index_per_row.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_partition" "../test-matrices/test_partition.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_some_empty_rows" "../test-matrices/test_some_empty_rows.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_tiny" "../test-matrices/test_tiny.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_tiny_odd" "../test-matrices/test_tiny_odd.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_tols90" "../test-matrices/test_tols90.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_tsopf1" "../test-matrices/test_tsopf1.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_tsopf2" "../test-matrices/test_tsopf2.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_two_rows" "../test-matrices/test_two_rows.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_break" "../test-matrices/test_break.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_bfwb62" "../test-matrices/bfwb62.mtx")

    AddMaxelerImplTestRun("test_spmv" "spmv_test_wa" "../test-matrices/test_wa.mtx")

    AddMaxelerImplTestRun("test_spmv" "spmv_OPF_3754" "../test-matrices/OPF_3754.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_OPF_6000" "../test-matrices/OPF_6000.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_test_cage6" "../test-matrices/test_cage6.mtx")
    AddMaxelerImplTestRun("test_spmv" "spmv_TSOPF_RS_b39_c7" "../test-matrices/TSOPF_RS_b39_c7.mtx")


    AddMaxelerImplTest("bicg" simbicglib "BiConjugateGradient_sim" "")
    AddMaxelerImplTestRun("test_bicg" "bicg" "")
    add_dependencies(test_bicg smpvlib)
    target_link_libraries(test_bicg -lSpmv_sim)

  endif()

endif()

