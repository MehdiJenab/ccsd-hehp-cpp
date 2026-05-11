add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE ccsd_config Catch2::Catch2WithMain)
ccsd_apply_flags(test_config)
catch_discover_tests(test_config PROPERTIES LABELS "unit")
