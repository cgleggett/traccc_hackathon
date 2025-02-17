enable_testing()

enable_language(CUDA)

macro(add_traccc_test TESTNAME FILES PLUGIN_LIBRARY)
    add_executable(${TESTNAME} ${FILES})
    target_link_libraries(${TESTNAME} gtest gmock gtest_main)
    target_link_libraries(${TESTNAME} traccc::core traccc::cuda)
    target_link_libraries(${TESTNAME} ${PLUGIN_LIBRARY})
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
    add_test(unit_test_${TESTNAME} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTNAME})
    set_tests_properties(unit_test_${TESTNAME} PROPERTIES 
    ENVIRONMENT TRACCC_TEST_DATA_DIR=${PROJECT_SOURCE_DIR}/data/)
endmacro()

add_subdirectory(algebra)
add_subdirectory(algorithms)
