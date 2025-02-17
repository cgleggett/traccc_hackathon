/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include <gtest/gtest.h>

// vecmem
#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/memory/cuda/managed_memory_resource.hpp>
#include "vecmem/memory/cuda/device_memory_resource.hpp"
#include "vecmem/memory/cuda/host_memory_resource.hpp"
#include "vecmem/utils/cuda/copy.hpp"

// traccc core
#include <edm/measurement.hpp>
#include <edm/track_parameters.hpp>
#include <edm/track_state.hpp>
#include <fitter/gain_matrix_updater.hpp>
#include <cuda/fitter/gain_matrix_updater.cuh>


// This defines the local frame test suite
TEST(algebra, gain_matrix_updater) {    
    const int batch_size = 1;

    // The managed memory resource
    vecmem::cuda::managed_memory_resource mng_mr;

    // The host/device memory resources
    vecmem::cuda::device_memory_resource dev_mr;
    vecmem::cuda::host_memory_resource host_mr;

    // memory copy helper
    vecmem::cuda::copy m_copy;
    
    // define track_state type
    using scalar_t = double;
    using measurement_t = traccc::measurement;
    using bound_track_parameters_t = traccc::bound_track_parameters;
    
    using track_state_t = traccc::track_state<measurement_t, bound_track_parameters_t>;
    using host_track_state_collection =
	traccc::host_track_state_collection<track_state_t>;

    // declare a test track states object
    host_track_state_collection track_states({host_track_state_collection::item_vector(&mng_mr)});
    
    // fillout random elements to track states
    for (int i_b=0; i_b < batch_size; i_b++){
	track_state_t tr_state;

	tr_state.predicted().vector() = bound_track_parameters_t::vector_t::Random();
	tr_state.predicted().covariance() = bound_track_parameters_t::covariance_t::Random();
	
	tr_state.filtered().vector() = bound_track_parameters_t::vector_t::Random();
	tr_state.filtered().covariance() = bound_track_parameters_t::covariance_t::Random();
	
	tr_state.smoothed().vector() = bound_track_parameters_t::vector_t::Random();
	tr_state.smoothed().covariance() = bound_track_parameters_t::covariance_t::Random();

	tr_state.jacobian() = track_state_t::jacobian_t::Random();
	tr_state.projector() = track_state_t::projector_t::Identity();	
	track_states.items.push_back(std::move(tr_state));
    }

    // cpu gain matrix updater
    traccc::gain_matrix_updater<track_state_t> cpu_updater;
    for (auto tr_state: track_states.items){
	cpu_updater.update(tr_state);
    }
    
    // cuda gain matrix updater
    traccc::cuda::gain_matrix_updater<track_state_t> cu_updater;
    cu_updater.update(track_states, &mng_mr);

    
}

// Google Test can be run manually from the main() function
// or, it can be linked to the gtest_main library for an already
// set-up main() function primed to accept Google Test test cases.
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
