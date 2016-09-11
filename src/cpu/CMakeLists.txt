find_package(Eigen REQUIRED)

add_library(mmio SHARED lib/mmio.c lib/mmio.h)

if (LibEIGEN_INCLUDE_DIRS)
  message(STATUS "Found EIGEN --> ${LibEIGEN_INCLUDE_DIRS}")
  include_directories(${LibEIGEN_INCLUDE_DIRS})
  add_executable(BicgStabEigen BicgStabEigen.cpp)
  add_executable(CgEigen CgEigen.cpp)
  target_link_libraries(CgEigen mmio)
endif()

find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  include_directories(lib)
  set(common_sources
    lib/mmio.c
    lib/common.c
    lib/timer.cpp
    )
  add_library(common ${common_sources})

  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES})
  add_executable(CgMklExplicit CgMklExplicit.cpp)
  add_executable(CgMklRci CgMklRci.cpp)
  add_executable(CgMklThreeTerm CgMklThreeTerm.cpp)
  target_link_libraries(CgMklExplicit common ${LIBS} -lrt)
  target_link_libraries(CgMklRci common ${LIBS} -lrt)
  target_link_libraries(CgMklThreeTerm common ${LIBS} -lrt)
endif()

