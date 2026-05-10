add_library(ccsd_config INTERFACE)
target_link_libraries(ccsd_config INTERFACE nlohmann_json::nlohmann_json)
target_include_directories(ccsd_config INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_config INTERFACE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
