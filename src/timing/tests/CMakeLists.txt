add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE ccsd_timing Catch2::Catch2WithMain)
ccsd_apply_flags(test_timer)
catch_discover_tests(test_timer PROPERTIES LABELS "unit")

add_executable(test_percentile test_percentile.cpp)
target_link_libraries(test_percentile PRIVATE ccsd_timing Catch2::Catch2WithMain)
ccsd_apply_flags(test_percentile)
catch_discover_tests(test_percentile PROPERTIES LABELS "unit")
