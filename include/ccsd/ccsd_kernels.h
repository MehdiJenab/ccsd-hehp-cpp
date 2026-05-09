#pragma once

#include <ccsd/ccsd_state.h>
#include <ccsd/parameters.h>

namespace ccsd {

// Pure math: implements the Stanton (1991) CCSD equations.
// No MPI calls. Caller is responsible for rank-gating.
class CcsdKernels {
public:
    CcsdKernels(CcsdState& state, const ParameterClass& p)
        : state_(state), p_(p) {}

    // Initialization
    void build_spin_integrals();   // <pq||rs> in spin-orbital basis
    void build_fock_spin();        // diagonal fock matrix in spin basis
    void guess_t2();               // MP2 initial guess for T2
    void build_denominators();     // Stanton eq. (12): denom_ai, denom_abij

    // CCSD intermediates (Stanton eqs. 3-8)
    void compute_F_ae();
    void compute_F_mi();
    void compute_F_me();
    void compute_W_mnij();
    void compute_W_abef();
    void compute_W_mbej();

    // Amplitude equations (Stanton eqs. 1-2)
    // Pre-condition: caller ensures this runs only on rank_master.
    void compute_t1();
    void compute_t2();

    // Energy expression (Crawford & Schaefer 2000, eq. 134/173)
    [[nodiscard]] double compute_energy() const;

    // Compound index hash — public for unit testing.
    [[nodiscard]] static double get_key(double a, double b, double c, double d);

private:
    CcsdState& state_;
    const ParameterClass& p_;

    [[nodiscard]] double get_value(double a, double b, double c, double d) const;
    [[nodiscard]] double taus(int a, int b, int i, int j) const;  // Stanton eq. (9)
    [[nodiscard]] double tau(int a, int b, int i, int j) const;   // Stanton eq. (10)
};

}  // namespace ccsd
