add_executable(test_kernels test_kernels.cpp)
target_link_libraries(test_kernels PRIVATE ccsd_kernels Catch2::Catch2WithMain)
ccsd_apply_flags(test_kernels)
# test_kernels loads config.json — run from build dir where it's copied.
catch_discover_tests(test_kernels
    PROPERTIES LABELS "unit"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_math_helpers test_math_helpers.cpp)
target_link_libraries(test_math_helpers PRIVATE ccsd_kernels Catch2::Catch2WithMain)
ccsd_apply_flags(test_math_helpers)
catch_discover_tests(test_math_helpers PROPERTIES LABELS "unit")
