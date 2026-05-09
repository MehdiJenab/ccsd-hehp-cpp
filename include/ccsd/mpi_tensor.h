#pragma once

#include <mpi.h>
#include <ccsd/tensors.h>
#include <ccsd/ccsd_constants.h>

namespace ccsd::mpi {

inline void send(Vector2D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, ccsd::constants::mpi_tag_2d, MPI_COMM_WORLD);
}
inline void recv(Vector2D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, ccsd::constants::mpi_tag_2d, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector2D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

inline void send(Vector4D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, ccsd::constants::mpi_tag_4d, MPI_COMM_WORLD);
}
inline void recv(Vector4D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, ccsd::constants::mpi_tag_4d, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector4D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

}  // namespace ccsd::mpi
