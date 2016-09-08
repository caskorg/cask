find_path(EIGEN_HEADER Eigen)

if (EIGEN_HEADER)
  message(STATUS " Found EIGEN --> ${EIGEN_HEADER}")
  include_directories(${EIGEN_HEADER})
  add_executable(BicgStabEigen BicgStabEigen.cpp)
endif()
