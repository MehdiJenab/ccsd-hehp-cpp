#pragma once

#include <kernels/ccsd_state.h>
#include <mpi/session.h>
#include <mpi/tensor_ops.h>

namespace ccsd {

class MpiOrchestrator {
public:
    MpiClass mpi;

    // Derives rank_start from size; must be called before any other method.
    void configure(int size, int rank) {
        mpi.size    = size;
        mpi.rank    = rank;
        rank_master_ = 0;
        rank_start_  = (size == 1) ? 0 : 1;
    }

    [[nodiscard]] int master() const noexcept { return rank_master_; }
    [[nodiscard]] int start()  const noexcept { return rank_start_; }

    void assign_tensor_owners(CcsdState& state) const {
        state.F_ae.rank = rank_start_;  state.F_me.rank = rank_start_;
        state.F_mi.rank = rank_start_;  state.W_mbej.rank = rank_start_;
        state.W_mnij.rank = rank_start_; state.W_abef.rank = rank_start_;
    }

    void gather_F_to_master(CcsdState& state) const {
        if (mpi.size == 1) return;
        send_2d(state.F_ae, state.F_ae.rank, rank_master_);
        send_2d(state.F_me, state.F_me.rank, rank_master_);
        send_2d(state.F_mi, state.F_mi.rank, rank_master_);
    }

    void gather_W_to_master(CcsdState& state) const {
        if (mpi.size == 1) return;
        send_4d(state.W_abef, state.W_abef.rank, rank_master_);
        send_4d(state.W_mbej, state.W_mbej.rank, rank_master_);
        send_4d(state.W_mnij, state.W_mnij.rank, rank_master_);
    }

    void broadcast_amplitudes(CcsdState& state) const {
        ccsd::mpi::bcast(state.t2_next, rank_master_);
        ccsd::mpi::bcast(state.t1_next, rank_master_);
    }

    void broadcast_scalar(double& value) const {
        // ccsd::mpi::bcast only handles Vector2D/4D; scalars go via raw MPI.
        MPI_Bcast(&value, 1, MPI_DOUBLE, rank_master_, MPI_COMM_WORLD);
    }

private:
    int rank_master_ = 0;
    int rank_start_  = 0;

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
