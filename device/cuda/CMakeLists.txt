enable_language(CUDA)

find_package(CUDAToolkit REQUIRED)

set( TRACCC_CUDA_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( TRACCC_CUDA_SEEDING_SOURCE_DIR ${TRACCC_CUDA_DIR}/src/seeding )
set( TRACCC_CUDA_UTILS_SOURCE_DIR ${TRACCC_CUDA_DIR}/src/utils )
set( TRACCC_CUDA_FITTER_SOURCE_DIR ${TRACCC_CUDA_DIR}/src/fitter )

add_subdirectory(include/cuda/utils)
add_subdirectory(include/cuda/seeding)
add_subdirectory(include/cuda/fitter)

add_library(traccc_cuda INTERFACE)
target_link_libraries(traccc_cuda INTERFACE
  traccc::cuda::utils
  traccc::cuda::seeding
  traccc::cuda::fitter)
add_library(traccc::cuda ALIAS traccc_cuda)
