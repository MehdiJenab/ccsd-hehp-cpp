#include <solver/ccsd_solver.h>
#include <kernels/ccsd_kernels.h>
#include <kernels/ccsd_constants.h>
#include <timing/timer.h>

#include <cmath>
#include <iostream>
#include <mpi.h>

namespace ccsd {

void CcsdSolver::initialization(CcsdKernels& kernels) {
    state_.allocate(2 * p.n_spatial_orbitals);
    orchestrator.assign_tensor_owners(state_);

    kernels.build_spin_integrals();
    kernels.build_fock_spin();
    kernels.guess_t2();
    kernels.build_denominators();
}

void CcsdSolver::compute_intermediates_distributed(CcsdKernels& kernels) {
    if (orchestrator.mpi.rank == state_.F_ae.rank)   kernels.compute_F_ae();
    if (orchestrator.mpi.rank == state_.F_mi.rank)   kernels.compute_F_mi();
    if (orchestrator.mpi.rank == state_.F_me.rank)   kernels.compute_F_me();
    if (orchestrator.mpi.rank == state_.W_mnij.rank) kernels.compute_W_mnij();
    if (orchestrator.mpi.rank == state_.W_abef.rank) kernels.compute_W_abef();
    if (orchestrator.mpi.rank == state_.W_mbej.rank) kernels.compute_W_mbej();
}

void CcsdSolver::solve_amplitudes_on_master(CcsdKernels& kernels) {
    // Gather F intermediates, then compute T1 on master.
    orchestrator.gather_F_to_master(state_);
    if (orchestrator.mpi.rank == orchestrator.master())
        kernels.compute_t1();

    // Gather W intermediates, then compute T2 on master.
    orchestrator.gather_W_to_master(state_);
    if (orchestrator.mpi.rank == orchestrator.master())
        kernels.compute_t2();
}

void CcsdSolver::run() {
    std::cout.precision(10);
    CcsdKernels kernels(state_, p);

    initialization(kernels);

    if (orchestrator.mpi.rank == orchestrator.master())
        std::cout << "CCSD in MpiC++" << std::endl;

    double cc_en = 0.0, cc_en_pre = 0.0, cc_en_diff = 10.0;

    // P13 audit: F/W intermediates scatter to per-rank workers, then gather to
    // rank_master for T1/T2 computation, then broadcast. Point-to-point is used
    // for the gather because only rank_master consumes the intermediates.
    while (cc_en_diff > constants::convergence_threshold) {
        cc_en_pre = cc_en;

        compute_intermediates_distributed(kernels);
        solve_amplitudes_on_master(kernels);

        orchestrator.broadcast_amplitudes(state_);
        state_.t2 = state_.t2_next;
        state_.t1 = state_.t1_next;

        if (orchestrator.mpi.rank == orchestrator.master()) {
            cc_en = kernels.compute_energy();
            cc_en_diff = std::abs(cc_en - cc_en_pre);
        }
        orchestrator.broadcast_scalar(cc_en_diff);
    }

    if (orchestrator.mpi.rank == orchestrator.master()) {
        std::cout << "  E(corr,CCSD) = " << cc_en << std::endl;
        std::cout << "  E(CCSD) = " << cc_en + p.nuclear_repulsion + p.hf_energy << std::endl;
    }
}

}  // namespace ccsd
