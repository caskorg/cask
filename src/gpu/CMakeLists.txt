find_package(CUDA)

if (CUDA_INCLUDE_DIRS)
  find_path(LibCUSP_INCLUDE_DIRS
    version.h
    lib/cusp/)
  if (LibCUSP_INCLUDE_DIRS)
    message(STATUS "CUSP --> ${LibCUSP_INCLUDE_DIRS}")
    include_directories(lib)
    cuda_add_executable(
      CgCusp
      CgCusp.cu
    )
  endif()
endif()
