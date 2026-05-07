#pragma once

#include <mpi.h>

#include "../../VectorsClass.h"  // T11 will switch this to <ccsd/tensors.hpp>

namespace ccsd::mpi {

inline void send(Vector2D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, 233, MPI_COMM_WORLD);
}
inline void recv(Vector2D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, 233, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector2D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

inline void send(Vector4D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, 610, MPI_COMM_WORLD);
}
inline void recv(Vector4D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, 610, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector4D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

}  // namespace ccsd::mpi
