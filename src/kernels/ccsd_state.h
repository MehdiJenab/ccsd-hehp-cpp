#pragma once

#include <tensors/vector_2d.h>
#include <tensors/vector_4d.h>

namespace ccsd {

// All tensor data for a CCSD calculation.
// Tensor::rank fields store the MPI owner for each intermediate — set by MpiOrchestrator.
struct CcsdState {
    Vector2D F_ae, F_mi, F_me;                          // Stanton eqs. 3-5 intermediates
    Vector4D W_mnij, W_abef, W_mbej;                    // Stanton eqs. 6-8 intermediates
    Vector2D t1, t1_next;                               // T1 amplitudes (current + next)
    Vector4D t2, t2_next;                               // T2 amplitudes (current + next)
    Vector2D denom_ai;                                  // Energy denominator, singles
    Vector4D denom_abij;                                // Energy denominator, doubles
    Vector2D fock_spin;                                 // Spin-basis Fock diagonal
    Vector4D spin_integrals;                            // <pq||rs> in spin-orbital basis
    int n_spin_orbitals = 0;

    void allocate(int n) {
        n_spin_orbitals = n;
        F_ae.initialization(n);  F_mi.initialization(n);  F_me.initialization(n);
        W_mnij.initialization(n); W_abef.initialization(n); W_mbej.initialization(n);
        t1.initialization(n);    t1_next.initialization(n);
        t2.initialization(n);    t2_next.initialization(n);
        denom_ai.initialization(n);  denom_abij.initialization(n);
        fock_spin.initialization(n);
        spin_integrals.initialization(n);
    }
};

}  // namespace ccsd
