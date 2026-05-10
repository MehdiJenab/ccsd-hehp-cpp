cmake_minimum_required(VERSION 3.21)

project(ccsd_hehp_cpp
    VERSION 1.0.0
    DESCRIPTION "CCSD for HeH+ with MPI parallelization — C++23"
    LANGUAGES CXX)

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Build options ─────────────────────────────────────────────────────────────
option(BUILD_TESTING        "Build unit and integration tests" ON)
option(BUILD_DOCUMENTATION  "Build API documentation with Doxygen" ON)
option(CCSD_ENABLE_ASAN     "AddressSanitizer"                  OFF)
option(CCSD_ENABLE_UBSAN    "UBSanitizer"                       OFF)
option(CCSD_ENABLE_TSAN     "ThreadSanitizer"                   OFF)
option(CCSD_ENABLE_COVERAGE "Code coverage"                     OFF)
option(CCSD_ENABLE_GPROF    "gprof instrumentation (-pg)"       OFF)
option(CCSD_USE_MDSPAN      "Use mdspan-backed tensors"         OFF)
option(CCSD_LAYOUT_ROW_MAJOR "Use row-major tensor layout"      OFF)
option(CCSD_USE_OMP         "OpenMP parallel-for in hot loops"  OFF)

# ── Compile definitions driven by options ─────────────────────────────────────
if(CCSD_USE_MDSPAN)
    add_compile_definitions(CCSD_USE_MDSPAN=1)
endif()
if(CCSD_LAYOUT_ROW_MAJOR)
    add_compile_definitions(CCSD_LAYOUT_ROW_MAJOR=1)
endif()
if(CCSD_USE_OMP)
    add_compile_definitions(CCSD_USE_OMP=1)
endif()

# ── CMake modules ─────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ProjectWarnings)
include(ccsd_mdspan)          # Kokkos mdspan reference impl (header-only)

# ── Per-target flag helper ────────────────────────────────────────────────────
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

# ── External dependencies ─────────────────────────────────────────────────────
find_package(MPI REQUIRED)
find_package(OpenMP)

if(CCSD_USE_OMP AND NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "CCSD_USE_OMP=ON but OpenMP was not found")
endif()

include(FetchContent)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(nlohmann_json)

if(BUILD_TESTING)
    enable_testing()
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
endif()

# ── Build tree ────────────────────────────────────────────────────────────────
add_subdirectory(src)
add_subdirectory(apps)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# ── Documentation ─────────────────────────────────────────────────────────────
if(BUILD_DOCUMENTATION)
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if(DOXYGEN_FOUND)
        configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile
                       ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
        message(STATUS "Doxygen found — build docs with 'cmake --build . --target docs'")
    else()
        message(STATUS "Doxygen not found — documentation target unavailable")
    endif()
endif()
