add_library(ccsd_mpi INTERFACE)
target_link_libraries(ccsd_mpi INTERFACE ccsd_tensors MPI::MPI_CXX)
target_include_directories(ccsd_mpi INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
target_compile_features(ccsd_mpi INTERFACE cxx_std_23)
