cmake_minimum_required(VERSION 3.12)
project(CCSD_Code VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTING "Build tests" ON)

# ── Pass-1 modernization additions ────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ProjectWarnings)
include(ccsd_mdspan)

option(CCSD_ENABLE_ASAN     "AddressSanitizer"  OFF)
option(CCSD_ENABLE_UBSAN    "UBSanitizer"       OFF)
option(CCSD_ENABLE_TSAN     "ThreadSanitizer"   OFF)
option(CCSD_ENABLE_COVERAGE "Code coverage"     OFF)
option(CCSD_ENABLE_GPROF    "Enable gprof instrumentation (-pg)" OFF)

option(CCSD_USE_MDSPAN "Use mdspan-backed tensors" OFF)

if(CCSD_USE_MDSPAN)
    add_compile_definitions(CCSD_USE_MDSPAN=1)
    link_libraries(ccsd::mdspan)
endif()

option(CCSD_LAYOUT_ROW_MAJOR "Use row-major layout for tensors" OFF)
if(CCSD_LAYOUT_ROW_MAJOR)
    add_compile_definitions(CCSD_LAYOUT_ROW_MAJOR=1)
endif()

function(ccsd_apply_flags TARGET)
    project_apply_warnings(${TARGET})
    if(CCSD_ENABLE_ASAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_options   (${TARGET} PUBLIC -fsanitize=address)
    endif()
    if(CCSD_ENABLE_UBSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=undefined)
        target_link_options   (${TARGET} PUBLIC -fsanitize=undefined)
    endif()
    if(CCSD_ENABLE_TSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=thread)
        target_link_options   (${TARGET} PUBLIC -fsanitize=thread)
    endif()
    if(CCSD_ENABLE_COVERAGE)
        target_compile_options(${TARGET} PUBLIC --coverage -O0 -g)
        target_link_options   (${TARGET} PUBLIC --coverage)
    endif()
    if(CCSD_ENABLE_GPROF)
        target_compile_options(${TARGET} PUBLIC -pg -O2 -g)
        target_link_options   (${TARGET} PUBLIC -pg)
    endif()
endfunction()

# Find MPI
find_package(MPI REQUIRED)

find_package(OpenMP)

option(CCSD_USE_OMP "Enable OpenMP parallel for in hot loops" OFF)
if(CCSD_USE_OMP)
    if(NOT OpenMP_CXX_FOUND)
        message(FATAL_ERROR "CCSD_USE_OMP=ON but OpenMP not found")
    endif()
    add_compile_definitions(CCSD_USE_OMP=1)
endif()

# nlohmann/json (header-only, fetched at configure)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Compiler flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Source files
set(SOURCES apps/ccsd_code.cpp)

# Create executable
add_executable(ccsd_code ${SOURCES})
ccsd_apply_flags(ccsd_code)

# Link MPI and nlohmann/json
target_link_libraries(ccsd_code PRIVATE MPI::MPI_CXX nlohmann_json::nlohmann_json $<$<BOOL:${CCSD_USE_OMP}>:OpenMP::OpenMP_CXX>)

# Include directories
target_include_directories(ccsd_code PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})

# Throughput-batch benchmark driver
add_executable(ccsd_bench apps/ccsd_bench.cpp)
ccsd_apply_flags(ccsd_bench)
target_link_libraries(ccsd_bench PRIVATE MPI::MPI_CXX nlohmann_json::nlohmann_json $<$<BOOL:${CCSD_USE_OMP}>:OpenMP::OpenMP_CXX>)
target_include_directories(ccsd_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})

# Installation
install(TARGETS ccsd_code
    RUNTIME DESTINATION bin
)

# Testing
if(BUILD_TESTING)
    enable_testing()
    
    # Expected results for validation
    set(EXPECTED_ECORR "-0.008225832259")
    set(EXPECTED_ECCSD "-2.862598243")
    
    # Copy config.json and other input files to build directory
    if(EXISTS "${CMAKE_SOURCE_DIR}/config.json")
        configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
        message(STATUS "Copied config.json to build directory")
    endif()
    
    # Copy any other input files that might exist
    file(GLOB INPUT_FILES "${CMAKE_SOURCE_DIR}/*.dat" 
         "${CMAKE_SOURCE_DIR}/*.inp" "${CMAKE_SOURCE_DIR}/input*")
    foreach(INPUT_FILE ${INPUT_FILES})
        get_filename_component(FILENAME ${INPUT_FILE} NAME)
        configure_file(${INPUT_FILE} ${CMAKE_BINARY_DIR}/${FILENAME} COPYONLY)
        message(STATUS "Copied ${FILENAME} to build directory")
    endforeach()
    
    # Test with different MPI process counts
    set(MPI_PROCESS_COUNTS 2 4 8)
    
    foreach(NP ${MPI_PROCESS_COUNTS})
        # Basic execution test
        add_test(
            NAME ccsd_test_np${NP}_execution
            COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} ${NP}
                    ${MPIEXEC_PREFLAGS} $<TARGET_FILE:ccsd_code>
        )
        
        set_tests_properties(ccsd_test_np${NP}_execution PROPERTIES
            TIMEOUT 60
            LABELS "execution"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        
        # Output validation test
        add_test(
            NAME ccsd_test_np${NP}_validation
            COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} ${NP}
                    ${MPIEXEC_PREFLAGS} $<TARGET_FILE:ccsd_code>
        )
        
        set_tests_properties(ccsd_test_np${NP}_validation PROPERTIES
            PASS_REGULAR_EXPRESSION "E\\(corr,CCSD\\) = ${EXPECTED_ECORR}.*E\\(CCSD\\) = ${EXPECTED_ECCSD}"
            TIMEOUT 60
            LABELS "validation"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endforeach()
    
    # Test with oversubscribe for systems with limited cores
    if(MPIEXEC_MAX_NUMPROCS)
        if(MPIEXEC_MAX_NUMPROCS LESS 8)
            add_test(
                NAME ccsd_test_np8_oversubscribe
                COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} 8
                        $<TARGET_FILE:ccsd_code>
            )
            
            set_tests_properties(ccsd_test_np8_oversubscribe PROPERTIES
                PASS_REGULAR_EXPRESSION "E\\(corr,CCSD\\) = ${EXPECTED_ECORR}.*E\\(CCSD\\) = ${EXPECTED_ECCSD}"
                TIMEOUT 90
                LABELS "oversubscribe"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
        endif()
    endif()
    
    # Python regression test (if available)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_mpi_regression.py")
        add_test(
            NAME ccsd_python_regression
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_mpi_regression.py
                    --executable $<TARGET_FILE:ccsd_code>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        
        set_tests_properties(ccsd_python_regression PROPERTIES
            TIMEOUT 120
            LABELS "regression"
        )
    endif()
    
    # Add custom test script if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.sh")
        add_test(
            NAME ccsd_shell_tests
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        
        set_tests_properties(ccsd_shell_tests PROPERTIES
            TIMEOUT 180
            LABELS "comprehensive"
        )
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "CCSD Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  MPI found: ${MPI_FOUND}")
message(STATUS "  MPI executable: ${MPIEXEC}")
message(STATUS "  JSON support: ${USE_JSON}")
if(USE_JSON AND JANSSON_LIBRARY)
    message(STATUS "  Jansson library: ${JANSSON_LIBRARY}")
endif()
message(STATUS "  Build testing: ${BUILD_TESTING}")
message(STATUS "")

#---------------------------------------------------------------------------
# Documentation with Doxygen
#---------------------------------------------------------------------------
option(BUILD_DOCUMENTATION "Build API documentation with Doxygen" ON)

if(BUILD_DOCUMENTATION)
    find_package(Doxygen
        OPTIONAL_COMPONENTS dot mscgen dia)
    
    if(DOXYGEN_FOUND)
        # Set Doxygen input and output directories
        set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR})
        set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
        
        # Copy Doxyfile to build directory
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile 
                       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        
        # Create output directory
        file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
        
        # Custom target to build documentation
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
        
        message(STATUS "Doxygen found - documentation can be built with 'make docs'")
        if(DOXYGEN_DOT_FOUND)
            message(STATUS "  Graphviz dot found - graphs will be generated")
        endif()
    else()
        message(WARNING "Doxygen not found - documentation cannot be built")
    endif()
endif()
# ── Catch2 unit tests (real tests added in later tickets) ─────────────────────
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    # Tensor unit tests
    add_executable(test_tensors tests/test_tensors.cpp)
    target_link_libraries(test_tensors PRIVATE Catch2::Catch2WithMain ccsd::mdspan)
    target_include_directories(test_tensors PRIVATE ${CMAKE_SOURCE_DIR}/include)
    ccsd_apply_flags(test_tensors)
    include(Catch)
    catch_discover_tests(test_tensors)

    # Parameter parsing tests
    add_executable(test_parameters tests/test_parameters.cpp)
    target_link_libraries(test_parameters PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
    target_include_directories(test_parameters PRIVATE ${CMAKE_SOURCE_DIR}/include)
    ccsd_apply_flags(test_parameters)
    catch_discover_tests(test_parameters)
endif()
