/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include <gtest/gtest.h>

#include <cuda/utils/definitions.hpp>

// vecmem
#include <vecmem/memory/cuda/managed_memory_resource.hpp>
#include "vecmem/memory/cuda/device_memory_resource.hpp"
#include "vecmem/memory/cuda/host_memory_resource.hpp"
#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/containers/vector.hpp>
#include "vecmem/utils/cuda/copy.hpp"

// cuda
#include <cublas_v2.h>
#include <cuda_runtime.h>

// acts
#include <Acts/Definitions/TrackParametrization.hpp>

// std
#include <chrono>

// This defines the local frame test suite
TEST(algebra, matrix_multiplication_tests) {

    // batch_size (number of matrices)
    const int batch_size = 5000;
    
    // matrix type
    using matrix_t = Acts::FreeSymMatrix;
    //using matrix_t = Acts::BoundSymMatrix;
    
    // for timing benchmark
    float gpu_mm(0); // mat-mul
    float cpu_mm(0);

    //float gpu_mi(0); // mat-inv
    //float cpu_mi(0);
    
    // declare cublas objects
    cublasHandle_t m_handle;
    cublasStatus_t m_status;

    // create cublas handler
    m_status = cublasCreate(&m_handle);

    // memory copy helper
    vecmem::cuda::copy m_copy;
    
    // The host/device memory resources
    vecmem::cuda::device_memory_resource dev_mr;
    vecmem::cuda::host_memory_resource host_mr;
    
    // alpha beta for cublas operation
    const double mm_alpha = 1; // mat-mul
    const double mm_beta = 0;

    //const double mi_alpha = 1; // mat-inv
    //const double mi_beta = 0;
    
    // generate test matrices        
    vecmem::vector< matrix_t > A_host(batch_size, &host_mr);
    vecmem::vector< matrix_t > B_host(batch_size, &host_mr);
    vecmem::vector< matrix_t > C_host(batch_size, &host_mr);    
    
    // Fill the host matrices
    for (size_t i_b = 0; i_b < batch_size; i_b++){
	A_host[i_b] = matrix_t::Random();
	B_host[i_b] = matrix_t::Random();
	C_host[i_b] = matrix_t::Zero();
    }

    // matrix size of BoundSymMatrix
    const int n_rows = A_host[0].rows();
    const int n_cols = A_host[0].cols();
    const int n_size = n_rows*n_cols;
    
    /*---------------------------------
      simple cpu eigen matrix operation
      ---------------------------------*/
    
    vecmem::vector< matrix_t > C_cpu(batch_size, &host_mr);    

    /*time*/ auto start_cpu_mm = std::chrono::system_clock::now();
    
    for (size_t i_b = 0; i_b < batch_size; i_b++){
	C_cpu[i_b] = A_host[i_b] * B_host[i_b];
    }

    /*time*/ auto end_cpu_mm = std::chrono::system_clock::now();
    /*time*/ std::chrono::duration<double> time_cpu_mm = end_cpu_mm - start_cpu_mm;

    /*time*/ cpu_mm += time_cpu_mm.count();

    /*----------------------------------------
      simple cuda kernel matrix multiplication
      ----------------------------------------*/
    
    /*------------------------------------
      cublas batched matrix multiplication
      ------------------------------------*/
    
    vecmem::vector< matrix_t > C_gpu(batch_size, &host_mr);    
    
    // Transfer data from host to device
    auto A_dev = m_copy.to ( vecmem::get_data( A_host ), dev_mr, vecmem::copy::type::host_to_device);
    auto B_dev = m_copy.to ( vecmem::get_data( B_host ), dev_mr, vecmem::copy::type::host_to_device);
    auto C_dev = m_copy.to ( vecmem::get_data( C_host ), dev_mr, vecmem::copy::type::host_to_device);
       
    // Assign pointer for cublas input
    vecmem::vector < double* > A_devPtr( batch_size, &host_mr);
    vecmem::vector < double* > B_devPtr( batch_size, &host_mr);
    vecmem::vector < double* > C_devPtr( batch_size, &host_mr);  
    
    for (size_t i_b = 0; i_b < batch_size; i_b++){	
	A_devPtr[i_b] = &A_dev.ptr()[i_b](0);
	B_devPtr[i_b] = &B_dev.ptr()[i_b](0);
	C_devPtr[i_b] = &C_dev.ptr()[i_b](0);
    }

    /*time*/ auto start_gpu_mm = std::chrono::system_clock::now();
    
    // Do the batched matrix multiplication
    cublasDgemmBatched(m_handle,
		       CUBLAS_OP_N, CUBLAS_OP_N,
		       n_rows, n_rows, n_rows,
		       &mm_alpha,
		       &A_devPtr[0], n_rows,
		       &B_devPtr[0], n_rows,
		       &mm_beta,
		       &C_devPtr[0], n_rows,
		       batch_size);

    /*time*/ auto end_gpu_mm = std::chrono::system_clock::now();
    /*time*/ std::chrono::duration<double> time_gpu_mm = end_gpu_mm - start_gpu_mm;

    /*time*/ gpu_mm += time_gpu_mm.count();
    
    CUDA_ERROR_CHECK(cudaGetLastError());
    CUDA_ERROR_CHECK(cudaDeviceSynchronize());
    
    // retrieve the result to the host
    for (size_t i_b = 0; i_b < batch_size; i_b++){	   
	m_copy( C_dev, C_gpu, vecmem::copy::type::device_to_host );
    }
    
    // destroy cublas handler
    m_status = cublasDestroy(m_handle);

    // Compare the cpu and gpu result
    for (size_t i_b = 0; i_b < batch_size; i_b++){
	//std::cout << C_cpu[i_b] << std::endl;
	//std::cout << C_gpu[i_b] << std::endl;
	
	for(size_t i_m = 0; i_m < n_size; ++i_m ) {
	    	    
	    EXPECT_TRUE( abs(C_cpu[i_b](i_m) - C_gpu[i_b](i_m)) < 1e-8 );
	}
    }
    
    std::cout << "==> Elpased time ... " << std::endl;
    std::cout << "cpu mat-mul time: " << cpu_mm << std::endl;
    std::cout << "gpu mat-mul time: " << gpu_mm << std::endl;
    std::cout << "mat-mul speedup: " << cpu_mm/gpu_mm << std::endl;
    
}


// Google Test can be run manually from the main() function
// or, it can be linked to the gtest_main library for an already
// set-up main() function primed to accept Google Test test cases.
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
