add_executable(ccsd_code ccsd_code.cpp)
ccsd_apply_flags(ccsd_code)
target_link_libraries(ccsd_code PRIVATE ccsd_core)

add_executable(ccsd_bench ccsd_bench.cpp)
ccsd_apply_flags(ccsd_bench)
target_link_libraries(ccsd_bench PRIVATE ccsd_core)

install(TARGETS ccsd_code RUNTIME DESTINATION bin)
