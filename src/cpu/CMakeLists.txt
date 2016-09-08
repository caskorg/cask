find_path(EIGEN_HEADER Eigen)

include_directories(lib)
set(common_sources
  lib/mmio.c
  lib/common.c
  lib/timer.cpp
)
add_library(common ${common_sources})

if (EIGEN_HEADER)
  message(STATUS "Found EIGEN --> ${EIGEN_HEADER}")
  include_directories(${EIGEN_HEADER})
  add_executable(BicgStabEigen BicgStabEigen.cpp)
endif()

find_package(MKL REQUIRED)
if (LibMKL_INCLUDE_DIRS)
  message(STATUS "Found MKL: ${LibMKL_INCLUDE_DIRS}")
  include_directories(${LibMKL_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${LibMKL_LIBRARIES})
  add_executable(CgMklExplicit CgMklExplicit.cpp)
  add_executable(CgMklRci CgMklRci.cpp)
  add_executable(CgMklThreeTerm CgMklThreeTerm.cpp)
  target_link_libraries(CgMklExplicit common ${LIBS} -lrt)
  target_link_libraries(CgMklRci common ${LIBS} -lrt)
  target_link_libraries(CgMklThreeTerm common ${LIBS} -lrt)
endif()

