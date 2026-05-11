add_executable(test_tensors test_tensors.cpp)
target_link_libraries(test_tensors PRIVATE
    ccsd_tensors ccsd::mdspan Catch2::Catch2WithMain)
ccsd_apply_flags(test_tensors)
catch_discover_tests(test_tensors PROPERTIES LABELS "unit")
