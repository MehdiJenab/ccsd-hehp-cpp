add_library(ccsd_solver STATIC ccsd_solver.cpp)
target_link_libraries(ccsd_solver PUBLIC
    ccsd_kernels ccsd_mpi ccsd_config ccsd_timing)
target_include_directories(ccsd_solver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_solver PUBLIC cxx_std_23)
ccsd_apply_flags(ccsd_solver)
