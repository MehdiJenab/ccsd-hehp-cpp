add_library(ccsd_kernels STATIC ccsd_kernels.cpp)
target_link_libraries(ccsd_kernels PUBLIC ccsd_tensors ccsd_config)
target_include_directories(ccsd_kernels PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_kernels PUBLIC cxx_std_23)
ccsd_apply_flags(ccsd_kernels)
if(CCSD_USE_OMP AND OpenMP_CXX_FOUND)
    target_link_libraries(ccsd_kernels PUBLIC OpenMP::OpenMP_CXX)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
