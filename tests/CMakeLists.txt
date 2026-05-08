include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# ── Integration / MPI tests ───────────────────────────────────────────────────
set(EXPECTED_ECORR "-0.008225832259")
set(EXPECTED_ECCSD "-2.862598243")

configure_file(${CMAKE_SOURCE_DIR}/config.json
               ${CMAKE_BINARY_DIR}/config.json COPYONLY)

foreach(NP 2 4 8)
    add_test(
        NAME ccsd_test_np${NP}_execution
        COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} ${NP}
                $<TARGET_FILE:ccsd_code>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(ccsd_test_np${NP}_execution PROPERTIES
        TIMEOUT 60 LABELS "execution")

    add_test(
        NAME ccsd_test_np${NP}_validation
        COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} ${NP}
                $<TARGET_FILE:ccsd_code>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(ccsd_test_np${NP}_validation PROPERTIES
        PASS_REGULAR_EXPRESSION
            "E\\(corr,CCSD\\) = ${EXPECTED_ECORR}.*E\\(CCSD\\) = ${EXPECTED_ECCSD}"
        TIMEOUT 60 LABELS "validation")
endforeach()

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(
        NAME ccsd_python_regression
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/tests/run_mpi_regression.py
                --executable $<TARGET_FILE:ccsd_code>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(ccsd_python_regression PROPERTIES
        TIMEOUT 120 LABELS "regression")
endif()

# ── Unit tests (Catch2) ───────────────────────────────────────────────────────
function(ccsd_unit_test NAME)
    add_executable(${NAME} ${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE
        ccsd_core Catch2::Catch2WithMain)
    ccsd_apply_flags(${NAME})
    catch_discover_tests(${NAME})
endfunction()

# test_tensors always needs the mdspan include path (for the smoke test)
add_executable(test_tensors test_tensors.cpp)
target_link_libraries(test_tensors PRIVATE ccsd_core Catch2::Catch2WithMain ccsd::mdspan)
ccsd_apply_flags(test_tensors)
catch_discover_tests(test_tensors)
ccsd_unit_test(test_parameters)
ccsd_unit_test(test_timer)
ccsd_unit_test(test_timing)
ccsd_unit_test(test_math_helpers)
