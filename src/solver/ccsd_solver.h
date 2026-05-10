#pragma once

#include <kernels/ccsd_state.h>
#include <kernels/ccsd_kernels.h>
#include <mpi/orchestrator.h>
#include <config/ccsd_config.h>

namespace ccsd {

// Coordinates initialization, MPI rank assignment, the CCSD iteration loop,
// and convergence checking. Owns CcsdState, CcsdKernels, and MpiOrchestrator.
class CcsdSolver {
public:
    ParameterClass p;
    MpiOrchestrator orchestrator;

    void attach(const MpiSession& session) {
        orchestrator.configure(session.size(), session.rank());
    }

    void run();

private:
    CcsdState state_;

    void initialization(CcsdKernels& kernels);
    void compute_intermediates_distributed(CcsdKernels& kernels);
    void solve_amplitudes_on_master(CcsdKernels& kernels);
};

}  // namespace ccsd

// Backwards compat: apps use unqualified CcsdSolver.
using CcsdSolver = ccsd::CcsdSolver;
