#include <ccsd/ccsd_kernels.h>

#include <cmath>
#include <vector>

#ifdef CCSD_USE_OMP
  #include <omp.h>
  #define CCSD_OMP_PARALLEL_FOR _Pragma("omp parallel for")
#else
  #define CCSD_OMP_PARALLEL_FOR
#endif

// Helpers for build_spin_integrals(): 1-indexed spin-orbital → 0-indexed spatial MO, spin parity.
static int spin_to_mo(int p)          { return (p + 1) / 2; }       // 1-based spin-orbital index p → 1-based spatial MO index (used with get_value's 1-based API).
static double same_spin(int p, int q) { return ((p % 2) == (q % 2)) ? 1.0 : 0.0; }
static double kronecker(int a, int b) { return (a == b) ? 1.0 : 0.0; }

//=============================================================================
double ccsd::CcsdKernels::get_key(double a, double b, double c, double d) { // Return compound index given four indices
    double ab, cd, abcd;
    if (a > b) {
        ab = a*(a+1)/2 + b;
    } else {
        ab = b*(b+1)/2 + a;
    }
    if (c > d) {
        cd = c*(c+1)/2 + d;
    } else {
        cd = d*(d+1)/2 + c;
    }
    if (ab > cd) {
        abcd = ab*(ab+1)/2 + cd;
    } else {
        abcd = cd*(cd+1)/2 + ab;
    }
    return abcd;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::get_value(double a, double b, double c, double d) const { // Return Value of spatial MO two electron integral
    // Example: (12\vert 34) = tei(1,2,3,4)
    double value;
    double key = get_key(a, b, c, d);
    if (p_.two_electron_mos.find(key) == p_.two_electron_mos.end()) {
        value = 0.0e0;
    } else {
        value = p_.two_electron_mos.find(key)->second;
    }
    return value;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::build_spin_integrals() { // CONVERT SPATIAL TO SPIN ORBITAL MO,
    // Build the spin-orbital two-electron integrals <pq||rs> = <pq|rs> - <pq|sr>
    // Indices pp,qq,rr,ss are 1-based spin orbitals; spin_to_mo converts to spatial MOs.
    state_.spin_integrals.zeros();
    for (int pp = 1; pp <= state_.n_spin_orbitals; ++pp) {
        for (int qq = 1; qq <= state_.n_spin_orbitals; ++qq) {
            for (int rr = 1; rr <= state_.n_spin_orbitals; ++rr) {
                for (int ss = 1; ss <= state_.n_spin_orbitals; ++ss) {
                    double direct   = get_value(spin_to_mo(pp), spin_to_mo(rr),
                                                spin_to_mo(qq), spin_to_mo(ss))
                                    * same_spin(pp,rr) * same_spin(qq,ss);
                    double exchange = get_value(spin_to_mo(pp), spin_to_mo(ss),
                                                spin_to_mo(qq), spin_to_mo(rr))
                                    * same_spin(pp,ss) * same_spin(qq,rr);
                    state_.spin_integrals(pp-1, qq-1, rr-1, ss-1) = direct - exchange;
                }
            }
        }
    }
}
//=============================================================================

//=============================================================================
void ccsd::CcsdKernels::build_fock_spin() { // Spin basis fock matrix eigenvalues, put MO energies in diagonal array
    std::vector<double> fock_1D(static_cast<std::size_t>(state_.n_spin_orbitals), 0.0);
    for (std::size_t i = 0; i < fock_1D.size(); ++i)
        fock_1D[i] = p_.orbital_energies[i / 2];
    state_.fock_spin.diagonalize(fock_1D);
}
//=============================================================================

//=============================================================================
void ccsd::CcsdKernels::guess_t2() {
    for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
        for (int b = p_.n_occupied; b < state_.n_spin_orbitals; ++b) {
            for (int i = 0; i < p_.n_occupied; ++i) {
                for (int j = 0; j < p_.n_occupied; ++j) {
                    state_.t2(a,b,i,j) += state_.spin_integrals(i,j,a,b)
                        / (state_.fock_spin(i,i) + state_.fock_spin(j,j)
                           - state_.fock_spin(a,a) - state_.fock_spin(b,b));
                }
            }
        }
    }
}
//=============================================================================

//=============================================================================
void ccsd::CcsdKernels::build_denominators() { // Make denominator arrays denom_ai, denom_abij, Equation (12) of Stanton
    state_.denom_ai.zeros();
    for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
        for (int i = 0; i < p_.n_occupied; ++i) {
            state_.denom_ai(a,i) = state_.fock_spin(i,i) - state_.fock_spin(a,a);
        }
    }
    state_.denom_abij.zeros();
    for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
        for (int b = p_.n_occupied; b < state_.n_spin_orbitals; ++b) {
            for (int i = 0; i < p_.n_occupied; ++i) {
                for (int j = 0; j < p_.n_occupied; ++j) {
                    state_.denom_abij(a,b,i,j) = state_.fock_spin(i,i) + state_.fock_spin(j,j)
                                               - state_.fock_spin(a,a) - state_.fock_spin(b,b);
                }
            }
        }
    }
}
//=============================================================================

//=============================================================================
double ccsd::CcsdKernels::tau_tilde(int a, int b, int i, int j) const { // Stanton eq (9)
    return state_.t2(a,b,i,j) + 0.5*(state_.t1(a,i)*state_.t1(b,j) - state_.t1(b,i)*state_.t1(a,j));
}
//=============================================================================

//=============================================================================
double ccsd::CcsdKernels::tau(int a, int b, int i, int j) const { // Stanton eq (10)
    return state_.t2(a,b,i,j) + state_.t1(a,i)*state_.t1(b,j) - state_.t1(b,i)*state_.t1(a,j);
}
//=============================================================================

//=============================================================================
void ccsd::CcsdKernels::compute_F_ae() { // Stanton eq (3)
    state_.F_ae.zeros();
    for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
        for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
            state_.F_ae(a,e) = (1.0 - kronecker(a,e)) * state_.fock_spin(a,e);
            for (int m = 0; m < p_.n_occupied; ++m) {
                state_.F_ae(a,e) += -0.5*state_.fock_spin(m,e)*state_.t1(a,m);
                for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                    state_.F_ae(a,e) += state_.t1(f,m)*state_.spin_integrals(m,a,f,e);
                    for (int n = 0; n < p_.n_occupied; ++n) {
                        state_.F_ae(a,e) += -0.5*tau_tilde(a,f,m,n)*state_.spin_integrals(m,n,e,f);
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_F_mi() { // Stanton eq (4)
    state_.F_mi.zeros();
    for (int m = 0; m < p_.n_occupied; ++m) {
        for (int i = 0; i < p_.n_occupied; ++i) {
            state_.F_mi(m,i) = (1.0 - kronecker(m,i)) * state_.fock_spin(m,i);
            for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
                state_.F_mi(m,i) += 0.5*state_.t1(e,i)*state_.fock_spin(m,e);
                for (int n = 0; n < p_.n_occupied; ++n) {
                    state_.F_mi(m,i) += state_.t1(e,n)*state_.spin_integrals(m,n,i,e);
                    for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                        state_.F_mi(m,i) += 0.5*tau_tilde(e,f,i,n)*state_.spin_integrals(m,n,e,f);
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_F_me() { // Stanton eq (5)
    state_.F_me.zeros();
    for (int m = 0; m < p_.n_occupied; ++m) {
        for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
            state_.F_me(m,e) = state_.fock_spin(m,e);
            for (int n = 0; n < p_.n_occupied; ++n) {
                for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                    state_.F_me(m,e) += state_.t1(f,n)*state_.spin_integrals(m,n,e,f);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_W_mnij() { // Stanton eq (6)
    state_.W_mnij.zeros();
    for (int m = 0; m < p_.n_occupied; ++m) {
        for (int n = 0; n < p_.n_occupied; ++n) {
            for (int i = 0; i < p_.n_occupied; ++i) {
                for (int j = 0; j < p_.n_occupied; ++j) {
                    state_.W_mnij(m,n,i,j) = state_.spin_integrals(m,n,i,j);
                    for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
                        state_.W_mnij(m,n,i,j) +=  state_.t1(e,j)*state_.spin_integrals(m,n,i,e)
                                                   -state_.t1(e,i)*state_.spin_integrals(m,n,j,e);
                        for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                            state_.W_mnij(m,n,i,j) += 0.25*tau(e,f,i,j)*state_.spin_integrals(m,n,e,f);
                        }
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_W_abef() { // Stanton eq (7)
    state_.W_abef.zeros();
    for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
        for (int b = p_.n_occupied; b < state_.n_spin_orbitals; ++b) {
            for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
                for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                    state_.W_abef(a,b,e,f) = state_.spin_integrals(a,b,e,f);
                    for (int m = 0; m < p_.n_occupied; ++m) {
                        state_.W_abef(a,b,e,f) += -state_.t1(b,m)*state_.spin_integrals(a,m,e,f)
                                                  +state_.t1(a,m)*state_.spin_integrals(b,m,e,f);
                        for (int n = 0; n < p_.n_occupied; ++n) {
                            state_.W_abef(a,b,e,f) += 0.25*tau(a,b,m,n)*state_.spin_integrals(m,n,e,f);
                        }
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_W_mbej() { // Stanton eq (8)
    state_.W_mbej.zeros();
    CCSD_OMP_PARALLEL_FOR
    for (int m = 0; m < p_.n_occupied; ++m) {
        for (int b = p_.n_occupied; b < state_.n_spin_orbitals; ++b) {
            for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e) {
                for (int j = 0; j < p_.n_occupied; ++j) {
                    state_.W_mbej(m,b,e,j) = state_.spin_integrals(m,b,e,j);
                    for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                        state_.W_mbej(m,b,e,j) += state_.t1(f,j)*state_.spin_integrals(m,b,e,f);
                    }
                    for (int n = 0; n < p_.n_occupied; ++n) {
                        state_.W_mbej(m,b,e,j) += -state_.t1(b,n)*state_.spin_integrals(m,n,e,j);
                        for (int f = p_.n_occupied; f < state_.n_spin_orbitals; ++f) {
                            state_.W_mbej(m,b,e,j) += -(0.5*state_.t2(f,b,j,n) + state_.t1(f,j)*state_.t1(b,n))
                                                       *state_.spin_integrals(m,n,e,f);
                        }
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t1_term_F_ae(int a, int i) const {
    double acc = 0.0;
    for (int e = p_.n_occupied; e < state_.n_spin_orbitals; ++e)
        acc += state_.t1(e,i)*state_.F_ae(a,e);
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t1_term_F_mi(int a, int i) const {
    double acc = 0.0;
    for (int m = 0; m < p_.n_occupied; ++m)
        acc += -state_.t1(a,m)*state_.F_mi(m,i);
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t1_terms_doubles(int a, int i) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int m = 0; m < n_occ; ++m) {
        for (int e = n_occ; e < n_so; ++e) {
            acc += state_.t2(a,e,i,m)*state_.F_me(m,e);                        // term 4
            for (int f = n_occ; f < n_so; ++f)
                acc += -0.5*state_.t2(e,f,i,m)*state_.spin_integrals(m,a,e,f); // term 5
            for (int n = 0; n < n_occ; ++n)
                acc += -0.5*state_.t2(a,e,m,n)*state_.spin_integrals(n,m,e,i); // term 6
        }
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t1_term_spinint(int a, int i) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int n = 0; n < n_occ; ++n)
        for (int f = n_occ; f < n_so; ++f)
            acc += -state_.t1(f,n)*state_.spin_integrals(n,a,i,f);
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_t1() { // Stanton eq (1)
    state_.t1_next.zeros();
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    CCSD_OMP_PARALLEL_FOR
    for (int a = n_occ; a < n_so; ++a) {
        for (int i = 0; i < n_occ; ++i) {
            double acc = state_.fock_spin(i, a)      // Stanton eq. (1), term 1: Fock off-diagonal
                       + t1_term_F_ae(a, i)           // term 2: T1·F_ae
                       + t1_term_F_mi(a, i)           // term 3: T1·F_mi
                       + t1_terms_doubles(a, i)       // terms 4–6: T2 dressed with F_me and spinints
                       + t1_term_spinint(a, i);       // term 7: T1·<na||if>
            state_.t1_next(a, i) = acc / state_.denom_ai(a, i);
        }
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_term_spinint(int a, int b, int i, int j) const {
    return state_.spin_integrals(i, j, a, b);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_terms_F_ae(int a, int b, int i, int j) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int e = n_occ; e < n_so; ++e) {
        acc += state_.t2(a,e,i,j)*state_.F_ae(b,e)
              -state_.t2(b,e,i,j)*state_.F_ae(a,e);
        for (int m = 0; m < n_occ; ++m) {
            acc += -0.5*state_.t2(a,e,i,j)*state_.t1(b,m)*state_.F_me(m,e)
                   +0.5*state_.t2(b,e,i,j)*state_.t1(a,m)*state_.F_me(m,e);
        }
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_terms_F_mi(int a, int b, int i, int j) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int m = 0; m < n_occ; ++m) {
        acc += -state_.t2(a,b,i,m)*state_.F_mi(m,j)
               +state_.t2(a,b,j,m)*state_.F_mi(m,i);
        for (int e = n_occ; e < n_so; ++e) {
            acc += -0.5*state_.t2(a,b,i,m)*state_.t1(e,j)*state_.F_me(m,e)
                   +0.5*state_.t2(a,b,j,m)*state_.t1(e,i)*state_.F_me(m,e);
        }
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_term_single_excitations(int a, int b, int i, int j) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int e = n_occ; e < n_so; ++e) {
        acc += state_.t1(e,i)*state_.spin_integrals(a,b,e,j)
              -state_.t1(e,j)*state_.spin_integrals(a,b,e,i);
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_term_W_abef(int a, int b, int i, int j) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int e = n_occ; e < n_so; ++e)
        for (int f = n_occ; f < n_so; ++f)
            acc += 0.5*tau(e,f,i,j)*state_.W_abef(a,b,e,f);
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_term_single_dressing(int a, int b, int i, int j) const {
    double acc = 0.0;
    for (int m = 0; m < p_.n_occupied; ++m) {
        acc += -state_.t1(a,m)*state_.spin_integrals(m,b,i,j)
               +state_.t1(b,m)*state_.spin_integrals(m,a,i,j);
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_terms_W_mbej(int a, int b, int i, int j) const {
    double acc = 0.0;
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    for (int m = 0; m < n_occ; ++m) {
        for (int e = n_occ; e < n_so; ++e) {
            acc +=  state_.t2(a,e,i,m)*state_.W_mbej(m,b,e,j) - state_.t1(e,i)*state_.t1(a,m)*state_.spin_integrals(m,b,e,j);
            acc += -state_.t2(a,e,j,m)*state_.W_mbej(m,b,e,i) + state_.t1(e,j)*state_.t1(a,m)*state_.spin_integrals(m,b,e,i);
            acc += -state_.t2(b,e,i,m)*state_.W_mbej(m,a,e,j) + state_.t1(e,i)*state_.t1(b,m)*state_.spin_integrals(m,a,e,j);
            acc +=  state_.t2(b,e,j,m)*state_.W_mbej(m,a,e,i) - state_.t1(e,j)*state_.t1(b,m)*state_.spin_integrals(m,a,e,i);
        }
    }
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double ccsd::CcsdKernels::t2_term_W_mnij(int a, int b, int i, int j) const {
    double acc = 0.0;
    for (int m = 0; m < p_.n_occupied; ++m)
        for (int n = 0; n < p_.n_occupied; ++n)
            acc += 0.5*tau(a,b,m,n)*state_.W_mnij(m,n,i,j);
    return acc;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void ccsd::CcsdKernels::compute_t2() { // Stanton eq (2)
    state_.t2_next.zeros();
    const int n_occ = p_.n_occupied;
    const int n_so  = state_.n_spin_orbitals;
    CCSD_OMP_PARALLEL_FOR
    for (int a = n_occ; a < n_so; ++a) {
        for (int b = n_occ; b < n_so; ++b) {
            for (int i = 0; i < n_occ; ++i) {
                for (int j = 0; j < n_occ; ++j) {
                    double acc = t2_term_spinint(a, b, i, j)
                               + t2_terms_F_ae(a, b, i, j)
                               + t2_terms_F_mi(a, b, i, j)
                               + t2_term_single_excitations(a, b, i, j)
                               + t2_term_W_abef(a, b, i, j)
                               + t2_term_single_dressing(a, b, i, j)
                               + t2_terms_W_mbej(a, b, i, j)
                               + t2_term_W_mnij(a, b, i, j);
                    state_.t2_next(a, b, i, j) = acc / state_.denom_abij(a, b, i, j);
                }
            }
        }
    }
}
//=============================================================================

//=============================================================================
double ccsd::CcsdKernels::compute_energy() const { // Equation (134) and (173); Expression from Crawford, Schaefer (2000)
    // DOI: 10.1002/9780470125915.ch2
    // computes CCSD energy given T1 and T2
    double ECCSD = 0.0;
    for (int i = 0; i < p_.n_occupied; ++i) {
        for (int a = p_.n_occupied; a < state_.n_spin_orbitals; ++a) {
            for (int j = 0; j < p_.n_occupied; ++j) {
                for (int b = p_.n_occupied; b < state_.n_spin_orbitals; ++b) {
                    ECCSD += 0.25*state_.spin_integrals(i,j,a,b)*state_.t2(a,b,i,j)
                           + 0.5*state_.spin_integrals(i,j,a,b)*state_.t1(a,i)*state_.t1(b,j);
                }
            }
        }
    }
    return ECCSD;
}
//=============================================================================
