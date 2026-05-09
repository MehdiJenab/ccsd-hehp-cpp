#pragma once

#include <ccsd/ccsd_state.h>
#include <ccsd/mpi_session.h>
#include <ccsd/mpi_tensor.h>

namespace ccsd {

// Manages all MPI communication and rank assignment for the CCSD solver.
class MpiOrchestrator {
public:
    MpiClass mpi;
    int rank_master = 0;
    int rank_start = 0;

    // Assign MPI owner ranks to each intermediate tensor in state.
    // With np=1: all on rank 0. With np>1: all intermediates on rank 1, master is rank 0.
    void assign_tensor_owners(CcsdState& state) const {
        state.F_ae.rank   = rank_start;
        state.F_me.rank   = rank_start;
        state.F_mi.rank   = rank_start;
        state.W_mbej.rank = rank_start;
        state.W_mnij.rank = rank_start;
        state.W_abef.rank = rank_start;
    }

    // Gather F intermediates from their owner rank to rank_master (needed before compute_t1).
    void gather_F_to_master(CcsdState& state) const {
        if (mpi.size > 1) {
            send_2d(state.F_ae, state.F_ae.rank, rank_master);
            send_2d(state.F_me, state.F_me.rank, rank_master);
            send_2d(state.F_mi, state.F_mi.rank, rank_master);
        }
    }

    // Gather W intermediates from their owner rank to rank_master (needed before compute_t2).
    void gather_W_to_master(CcsdState& state) const {
        if (mpi.size > 1) {
            send_4d(state.W_abef, state.W_abef.rank, rank_master);
            send_4d(state.W_mbej, state.W_mbej.rank, rank_master);
            send_4d(state.W_mnij, state.W_mnij.rank, rank_master);
        }
    }

    // Broadcast updated amplitudes from rank_master to all ranks.
    void broadcast_amplitudes(CcsdState& state) const {
        ccsd::mpi::bcast(state.t2_next, rank_master);
        ccsd::mpi::bcast(state.t1_next, rank_master);
    }

    // Broadcast a scalar double from rank 0 to all ranks.
    void broadcast_scalar(double& value) const {
        MPI_Bcast(&value, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

private:
    void send_2d(Vector2D& t, int src, int dst) const {
        if (mpi.rank == src) ccsd::mpi::send(t, dst);
        if (mpi.rank == dst) ccsd::mpi::recv(t, src);
    }

    void send_4d(Vector4D& t, int src, int dst) const {
        if (mpi.rank == src) ccsd::mpi::send(t, dst);
        if (mpi.rank == dst) ccsd::mpi::recv(t, src);
    }
};

}  // namespace ccsd
