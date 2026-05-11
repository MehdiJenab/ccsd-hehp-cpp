add_library(ccsd_tensors INTERFACE)
target_include_directories(ccsd_tensors INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_tensors INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
