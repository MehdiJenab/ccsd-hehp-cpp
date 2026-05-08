add_library(ccsd_core STATIC ccsd_solver.cpp)

target_include_directories(ccsd_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(ccsd_core PUBLIC
    MPI::MPI_CXX
    nlohmann_json::nlohmann_json)

if(CCSD_USE_OMP AND OpenMP_CXX_FOUND)
    target_link_libraries(ccsd_core PUBLIC OpenMP::OpenMP_CXX)
endif()

if(CCSD_USE_MDSPAN)
    target_link_libraries(ccsd_core PUBLIC ccsd::mdspan)
endif()

target_compile_features(ccsd_core PUBLIC cxx_std_23)

ccsd_apply_flags(ccsd_core)
