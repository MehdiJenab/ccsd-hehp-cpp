add_library(ccsd_timing INTERFACE)
target_include_directories(ccsd_timing INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_timing INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
