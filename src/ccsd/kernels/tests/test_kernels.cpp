#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ccsd/kernels/ccsd_kernels.h>
#include <ccsd/kernels/ccsd_state.h>
#include <ccsd/kernels/ccsd_constants.h>
#include <ccsd/config/ccsd_config.h>

using Catch::Approx;

// ── helpers ──────────────────────────────────────────────────────────────────

// Build a minimal 4-spin-orbital state (HeH+ has dim=2 → 4 spin orbitals,
// n_occupied=2). Orbital energies from the standard HeH+/STO-3G input.
static ccsd::CcsdConfig make_hehp_config() {
    ccsd::CcsdConfig c{ccsd::CcsdConfig::direct_init{}};
    c.n_spatial_orbitals = 2;
    c.n_occupied         = 2;
    c.orbital_energies   = {-0.913, 1.395};
    c.nuclear_repulsion  = 1.3;
    c.hf_energy          = -2.854;
    // two_electron_mos left empty for tests that don't need them
    return c;
}

static ccsd::CcsdState make_allocated_state(int n_spatial) {
    ccsd::CcsdState s;
    s.allocate(2 * n_spatial);
    return s;
}

// ── tau_tilde / tau ──────────────────────────────────────────────────────────

TEST_CASE("tau_tilde equals half T1-antisymmetric product when T2=0", "[kernels][tau]") {
    auto cfg = make_hehp_config();
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);
    // Virtual indices: 2, 3. Occupied indices: 0, 1.
    s.t1(2, 0) = 0.1;  s.t1(2, 1) = 0.2;
    s.t1(3, 0) = 0.3;  s.t1(3, 1) = 0.4;
    // T2 is zero (default).

    ccsd::CcsdKernels k(s, cfg);

    // tau_tilde(a,b,i,j) = T2 + 0.5*(T1[a,i]*T1[b,j] - T1[b,i]*T1[a,j])
    double t1_part = 0.1 * 0.4 - 0.3 * 0.2;  // -0.02
    REQUIRE(k.tau_tilde(2, 3, 0, 1) == Approx(0.5 * t1_part));
}

TEST_CASE("tau equals twice tau_tilde when T2=0", "[kernels][tau]") {
    auto cfg = make_hehp_config();
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);
    s.t1(2, 0) = 0.1;  s.t1(2, 1) = 0.2;
    s.t1(3, 0) = 0.3;  s.t1(3, 1) = 0.4;

    ccsd::CcsdKernels k(s, cfg);
    // tau = T2 + T1*T1 antisymm = 2 * tau_tilde when T2=0
    REQUIRE(k.tau(2, 3, 0, 1) == Approx(2.0 * k.tau_tilde(2, 3, 0, 1)));
}

TEST_CASE("tau_tilde and tau include T2 contribution", "[kernels][tau]") {
    auto cfg = make_hehp_config();
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);
    s.t2(2, 3, 0, 1) = 0.5;
    // T1 = 0, so tau = T2 and tau_tilde = T2.

    ccsd::CcsdKernels k(s, cfg);
    REQUIRE(k.tau(2, 3, 0, 1)       == Approx(0.5));
    REQUIRE(k.tau_tilde(2, 3, 0, 1) == Approx(0.5));
}

// ── build_fock_spin ──────────────────────────────────────────────────────────

TEST_CASE("build_fock_spin fills diagonal with orbital energies (each appears twice)", "[kernels][fock]") {
    auto cfg = make_hehp_config();  // orbital_energies = {-0.913, 1.395}
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);

    ccsd::CcsdKernels k(s, cfg);
    k.build_fock_spin();

    // n_spin_orbitals = 4; spin orbitals 0,1 map to spatial MO 0 (energy -0.913),
    // spin orbitals 2,3 map to spatial MO 1 (energy 1.395).
    REQUIRE(s.fock_spin(0, 0) == Approx(-0.913));
    REQUIRE(s.fock_spin(1, 1) == Approx(-0.913));
    REQUIRE(s.fock_spin(2, 2) == Approx(1.395));
    REQUIRE(s.fock_spin(3, 3) == Approx(1.395));
    // Off-diagonal must be zero.
    REQUIRE(s.fock_spin(0, 1) == Approx(0.0));
    REQUIRE(s.fock_spin(0, 2) == Approx(0.0));
}

// ── build_denominators ───────────────────────────────────────────────────────

TEST_CASE("build_denominators: denom_ai = fock(i,i) - fock(a,a)", "[kernels][denom]") {
    auto cfg = make_hehp_config();
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);

    ccsd::CcsdKernels k(s, cfg);
    k.build_fock_spin();
    k.build_denominators();

    // Virtual a=2, occupied i=0: fock(0,0)-fock(2,2) = -0.913 - 1.395 = -2.308
    REQUIRE(s.denom_ai(2, 0) == Approx(-0.913 - 1.395));
    REQUIRE(s.denom_ai(3, 1) == Approx(-0.913 - 1.395));
    // Doubles: denom_abij(2,3,0,1) = fock(0)+fock(1)-fock(2)-fock(3)
    REQUIRE(s.denom_abij(2, 3, 0, 1) == Approx(2 * (-0.913 - 1.395)));
}

// ── compute_energy ───────────────────────────────────────────────────────────

TEST_CASE("compute_energy returns zero when all amplitudes are zero", "[kernels][energy]") {
    auto cfg = make_hehp_config();
    auto s   = make_allocated_state(cfg.n_spatial_orbitals);
    // All tensors zero-initialized by allocate().

    ccsd::CcsdKernels k(s, cfg);
    REQUIRE(k.compute_energy() == Approx(0.0));
}

// ── full HeH+ kernel loop (no MPI) ──────────────────────────────────────────

TEST_CASE("CcsdKernels converges HeH+ to reference energy without MPI", "[kernels][integration]") {
    // Load the real HeH+/STO-3G parameters.
    ccsd::CcsdConfig cfg("./config.json");
    ccsd::CcsdState  s;
    s.allocate(2 * cfg.n_spatial_orbitals);

    ccsd::CcsdKernels k(s, cfg);
    k.build_spin_integrals();
    k.build_fock_spin();
    k.guess_t2();
    k.build_denominators();

    double energy = 0.0, energy_prev = 0.0, diff = 10.0;
    int iter = 0;
    while (diff > ccsd::constants::convergence_threshold && iter < 200) {
        energy_prev = energy;
        k.compute_F_ae();  k.compute_F_mi();  k.compute_F_me();
        k.compute_W_mnij(); k.compute_W_abef(); k.compute_W_mbej();
        k.compute_t1();
        k.compute_t2();
        s.t1 = s.t1_next;
        s.t2 = s.t2_next;
        energy = k.compute_energy();
        diff = std::abs(energy - energy_prev);
        ++iter;
    }

    // Must converge and match the known HeH+/STO-3G CCSD correlation energy.
    REQUIRE(iter < 200);
    REQUIRE(energy == Approx(-0.008225832259).epsilon(1e-8));
}
