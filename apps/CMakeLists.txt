add_executable(ccsd_code ccsd_code.cpp)
target_link_libraries(ccsd_code PRIVATE ccsd_solver)
ccsd_apply_flags(ccsd_code)

add_executable(ccsd_bench ccsd_bench.cpp)
target_link_libraries(ccsd_bench PRIVATE ccsd_solver ccsd_timing)
ccsd_apply_flags(ccsd_bench)

install(TARGETS ccsd_code RUNTIME DESTINATION bin)
