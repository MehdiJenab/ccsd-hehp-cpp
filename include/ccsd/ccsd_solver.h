#ifndef CCSD_CCSD_SOLVER_H
#define CCSD_CCSD_SOLVER_H

#include <ccsd/mpi_session.h>
#include <ccsd/parameters.h>
#include <ccsd/tensors.h>

class CcsdSolver {
public:
    ParameterClass p;
    MpiClass mpi;
    int rank_master = 0;
    int rank_start = 0;
    Vector2D F_ae, F_mi, F_me, denom_ai, t1_next, t1, fock_spin;
    Vector4D W_mnij, W_abef, W_mbej, denom_abij, t2_next, t2, spin_integrals;
    int n_spin_orbitals = 0;

    void run();

    // Pure math helper — no member state; accessible for unit testing.
    [[nodiscard]] static double get_key(double a, double b, double c, double d);

private:
    void guess_T2();
    void get_denominator_arrays();
    [[nodiscard]] double get_value(double a, double b, double c, double d) const;
    void get_spinints();
    void get_fs();
    int update(int i);
    void get_ranks();
    void initialization_vectors();
    void initialization();
    [[nodiscard]] double get_taus(int a, int b, int i, int j) const;
    [[nodiscard]] double get_tau(int a, int b, int i, int j) const;
    void get_Fae();
    void get_Fmi();
    void get_Fme();
    void get_Wmnij();
    void get_Wabef();
    void get_Wmbej();
    void update_intermediates();
    void sendVec2D(Vector2D& vec2D, int rank_src, int rank_dst);
    void sendVec4D(Vector4D& vec4D, int rank_src, int rank_dst);
    void makeT1_s();
    void makeT2_d();
    [[nodiscard]] double ccsdenergy() const;
};

#endif  // CCSD_CCSD_SOLVER_H
