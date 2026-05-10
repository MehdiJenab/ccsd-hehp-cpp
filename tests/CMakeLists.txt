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
        TIMEOUT 60 LABELS "integration;execution")

    add_test(
        NAME ccsd_test_np${NP}_validation
        COMMAND ${MPIEXEC} --oversubscribe ${MPIEXEC_NUMPROC_FLAG} ${NP}
                $<TARGET_FILE:ccsd_code>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(ccsd_test_np${NP}_validation PROPERTIES
        PASS_REGULAR_EXPRESSION
            "E\\(corr,CCSD\\) = ${EXPECTED_ECORR}.*E\\(CCSD\\) = ${EXPECTED_ECCSD}"
        TIMEOUT 60 LABELS "integration;validation")
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
        TIMEOUT 120 LABELS "integration;regression")
endif()
