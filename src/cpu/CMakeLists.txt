find_package(Eigen REQUIRED)

add_library(mmio SHARED lib/mmio.c lib/mmio.h)

if (LibEIGEN_INCLUDE_DIRS)
  message(STATUS "Found EIGEN --> ${LibEIGEN_INCLUDE_DIRS}")
  include_directories(lib/)
  include_directories(${LibEIGEN_INCLUDE_DIRS})
  add_executable(BicgStabEigen BicgStabEigen.cpp EigenUtils.hpp BenchmarkUtils.hpp)
  add_executable(CgEigen CgEigen.cpp EigenUtils.hpp BenchmarkUtils.hpp)
  target_link_libraries(CgEigen mmio)
  target_link_libraries(BicgStabEigen mmio)
endif()

find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  include_directories(lib)
  set(common_sources
    lib/mmio.c
    lib/common.c
    )
  add_library(common ${common_sources})

  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES} common -lrt -fopenmp)
  add_executable(CgMklExplicit CgMklExplicit.cpp)
  add_executable(CgMklRci CgMklRci.cpp)
  add_executable(CgMklThreeTerm CgMklThreeTerm.cpp)
  target_link_libraries(CgMklExplicit ${LIBS})
  target_link_libraries(CgMklRci ${LIBS})
  target_link_libraries(CgMklThreeTerm ${LIBS})
endif()

