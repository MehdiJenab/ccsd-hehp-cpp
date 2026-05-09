#include <ccsd/ccsd_solver.h>
#include <ccsd/ccsd_constants.h>
#include <ccsd/mpi_tensor.h>
#include <ccsd/timer.h>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <vector>

#include <mpi.h>

#ifdef CCSD_USE_OMP
  #include <omp.h>
  #define CCSD_OMP_PARALLEL_FOR _Pragma("omp parallel for")
#else
  #define CCSD_OMP_PARALLEL_FOR
#endif

using namespace std;  // acceptable in .cpp

// Helpers for get_spinints(): 1-indexed spin-orbital → 0-indexed spatial MO, spin parity.
static int spin_to_mo(int p)        { return (p + 1) / 2; }       // 1-based spin-orbital index p → 1-based spatial MO index (used with get_value's 1-based API).
static double same_spin(int p, int q) { return ((p % 2) == (q % 2)) ? 1.0 : 0.0; }
static double kronecker(int a, int b){ return (a == b) ? 1.0 : 0.0; }

//=============================================================================
void CcsdSolver::guess_T2(){
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a){
		for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
			for (int i = 0; i < p.n_occupied; ++i){
				for (int j = 0; j < p.n_occupied; ++j){
					t2(a,b,i,j) += spin_integrals(i,j,a,b)/(fock_spin(i,i) + fock_spin(j,j) - fock_spin(a,a) - fock_spin(b,b));
				}
			}
		}
	}
}
//=============================================================================

//=============================================================================
void CcsdSolver::get_denominator_arrays(){ // Make denominator arrays denom_ai, denom_abij, Equation (12) of Stanton

	denom_ai.zeros();
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a) {
		for (int i = 0; i < p.n_occupied; ++i){
			denom_ai(a,i) = fock_spin(i,i) - fock_spin(a,a);
		}
	}
	denom_abij.zeros();
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a) {
		for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
			for (int i = 0; i < p.n_occupied; ++i){
				for (int j = 0; j < p.n_occupied; ++j){
					denom_abij(a,b,i,j) = fock_spin(i,i) + fock_spin(j,j) - fock_spin(a,a) - fock_spin(b,b);
				}
			}
		}
	}
}
//=============================================================================


//=============================================================================
double CcsdSolver::get_key(double a, double b, double c, double d) { // Return compound index given four indices
	double ab,cd,abcd;
	if (a > b){
		ab = a*(a+1)/2 + b;
	}else{
		ab = b*(b+1)/2 + a;
	}
	if (c > d){
		cd = c*(c+1)/2 + d;
	}else{
		cd = d*(d+1)/2 + c;
	}
	if (ab > cd){
		abcd = ab*(ab+1)/2 + cd;
	}else{
		abcd = cd*(cd+1)/2 + ab;
	}
	return abcd;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
double CcsdSolver::get_value(double a, double b, double c, double d) const { // Return Value of spatial MO two electron integral
	// Example: (12\vert 34) = tei(1,2,3,4)
	double value;
	double key = get_key(a,b,c,d);
	if ( p.two_electron_mos.find(key) == p.two_electron_mos.end() ) {
		value=0.0e0;
	} else {
		value = p.two_electron_mos.find(key)->second;
	}
	return  value;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CcsdSolver::get_spinints(){ // CONVERT SPATIAL TO SPIN ORBITAL MO,
	// Build the spin-orbital two-electron integrals <pq||rs> = <pq|rs> - <pq|sr>
	// Indices pp,qq,rr,ss are 1-based spin orbitals; spin_to_mo converts to spatial MOs.
	spin_integrals.zeros();
	for (int pp = 1; pp <= n_spin_orbitals; ++pp) {
		for (int qq = 1; qq <= n_spin_orbitals; ++qq) {
			for (int rr = 1; rr <= n_spin_orbitals; ++rr) {
				for (int ss = 1; ss <= n_spin_orbitals; ++ss) {
					double direct   = get_value(spin_to_mo(pp), spin_to_mo(rr),
					                            spin_to_mo(qq), spin_to_mo(ss))
					                * same_spin(pp,rr) * same_spin(qq,ss);
					double exchange = get_value(spin_to_mo(pp), spin_to_mo(ss),
					                            spin_to_mo(qq), spin_to_mo(rr))
					                * same_spin(pp,ss) * same_spin(qq,rr);
					spin_integrals(pp-1, qq-1, rr-1, ss-1) = direct - exchange;
				}
			}
		}
	}
}
//=============================================================================


//=============================================================================
void CcsdSolver::get_fs(){//Spin basis fock matrix eigenvalues, put MO energies in diagonal array
	vector<double> fock_1D;
	fock_1D.resize(static_cast<std::size_t>(n_spin_orbitals));
	fill(fock_1D.begin(), fock_1D.end(), 0.0);

	for (std::size_t i = 0; i < fock_1D.size(); ++i){
		fock_1D[i] = p.orbital_energies[i / 2];
	}

	fock_spin.diagonalize(fock_1D);

}
//=============================================================================


//=============================================================================
int CcsdSolver::update(int i){
	i++ ;
	if (i>0){
		i=rank_start;
	}
	return i;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CcsdSolver::get_ranks(){
	rank_master = 0;
	int i=rank_start;
	F_ae.rank = i;
	i = update(i);
	F_me.rank = i;
	i = update(i);
	F_mi.rank = i;
	i = update(i);
	W_mbej.rank = i;
	i = update(i);
	W_mnij.rank = i;
	i = update(i);
	W_abef.rank = i;
}
//=============================================================================

//-----------------------------------------------------------------------------
void CcsdSolver::initialization_vectors(){
	F_ae.initialization(n_spin_orbitals);
	F_mi.initialization(n_spin_orbitals);
	F_me.initialization(n_spin_orbitals);
	denom_ai.initialization(n_spin_orbitals);
	t1_next.initialization(n_spin_orbitals);
	t1.initialization(n_spin_orbitals);
	fock_spin.initialization(n_spin_orbitals);

	W_mnij.initialization(n_spin_orbitals);
	W_abef.initialization(n_spin_orbitals);
	W_mbej.initialization(n_spin_orbitals);
	denom_abij.initialization(n_spin_orbitals);
	t2_next.initialization(n_spin_orbitals);
	t2.initialization(n_spin_orbitals);
	spin_integrals.initialization(n_spin_orbitals);;
}
//-----------------------------------------------------------------------------

//=============================================================================
void CcsdSolver::initialization(){
	// MPI lifetime is now owned by ccsd::MpiSession in main().
	#ifdef timing
 	if (mpi.rank==rank_master)	ccsd::Timer timer("initialization",mpi.rank);
	#endif

	n_spin_orbitals = 2 * p.n_spatial_orbitals;

	// mpi starts ...

	initialization_vectors();


	if (mpi.size==1){
		rank_start = 0;
	}else{
		rank_start = 1;
	}

	get_ranks();

	get_spinints();
	get_fs();
	guess_T2();
	get_denominator_arrays();
}
//=============================================================================




////////////////////////////////////////////////////////////////////////////////
//
// CCSD CALCULATION
//
////////////////////////////////////////////////////////////////////////////////

//=============================================================================
double CcsdSolver::get_taus(int a, int b, int i, int j) const {// Stanton eq (9)
	double taus = t2(a,b,i,j) + 0.5*(t1(a,i)*t1(b,j) - t1(b,i)*t1(a,j));
	return taus;
}
//=============================================================================


//=============================================================================
double CcsdSolver::get_tau(int a, int b, int i, int j) const { // Stanton eq (10)
	double tau=t2(a,b,i,j) + t1(a,i)*t1(b,j) - t1(b,i)*t1(a,j);
	return tau;
}
//=============================================================================


//=============================================================================
void CcsdSolver::get_Fae(){ // Stanton eq (3)
	F_ae.zeros();
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a) {
		for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
			F_ae(a,e) = (1.0 - kronecker(a,e)) * fock_spin(a,e);
			for (int m = 0; m < p.n_occupied; ++m){
				F_ae(a,e) += -0.5*fock_spin(m,e)*t1(a,m);
				for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
					F_ae(a,e) += t1(f,m)*spin_integrals(m,a,f,e);
					for (int n = 0; n < p.n_occupied; ++n){
						F_ae(a,e) += -0.5*get_taus(a,f,m,n)*spin_integrals(m,n,e,f);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Fmi(){ // Stanton eq (4)
	F_mi.zeros();
	for (int m = 0; m < p.n_occupied; ++m) {
		for (int i = 0; i < p.n_occupied; ++i) {
			F_mi(m,i) = (1.0 - kronecker(m,i)) * fock_spin(m,i);
			for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
				F_mi(m,i) += 0.5*t1(e,i)*fock_spin(m,e);
				for (int n = 0; n < p.n_occupied; ++n){
					F_mi(m,i) += t1(e,n)*spin_integrals(m,n,i,e);
					for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
						F_mi(m,i) += 0.5*get_taus(e,f,i,n)*spin_integrals(m,n,e,f);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Fme(){  // Stanton eq (5)
	F_me.zeros();
	for (int m = 0; m < p.n_occupied; ++m){
		for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
			F_me(m,e) = fock_spin(m,e);
			for (int n = 0; n < p.n_occupied; ++n){
				for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
					F_me(m,e) += t1(f,n)*spin_integrals(m,n,e,f);
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Wmnij(){// Stanton eq (6)
	W_mnij.zeros();
	for (int m = 0; m < p.n_occupied; ++m){
		for (int n = 0; n < p.n_occupied; ++n){
			for (int i = 0; i < p.n_occupied; ++i){
				for (int j = 0; j < p.n_occupied; ++j){
					W_mnij(m,n,i,j) = spin_integrals(m,n,i,j);
					for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
						W_mnij(m,n,i,j) +=  t1(e,j)*spin_integrals(m,n,i,e)
											-t1(e,i)*spin_integrals(m,n,j,e);
						for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
							W_mnij(m,n,i,j) += 0.25*get_tau(e,f,i,j)*spin_integrals(m,n,e,f);
						}
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CcsdSolver::get_Wabef(){ // Stanton eq (7)
	W_abef.zeros();
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a){
		for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
			for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
				for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
					W_abef(a,b,e,f) = spin_integrals(a,b,e,f);
					for (int m = 0; m < p.n_occupied; ++m){
						W_abef(a,b,e,f) += -t1(b,m)*spin_integrals(a,m,e,f)
											+t1(a,m)*spin_integrals(b,m,e,f);
						for (int n = 0; n < p.n_occupied; ++n){
							W_abef(a,b,e,f) += 0.25*get_tau(a,b,m,n)*spin_integrals(m,n,e,f);
						}
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Wmbej(){ // Stanton eq (8)
	W_mbej.zeros();
	CCSD_OMP_PARALLEL_FOR
	for (int m = 0; m < p.n_occupied; ++m){
		for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
			for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
				for (int j = 0; j < p.n_occupied; ++j){
					W_mbej(m,b,e,j) = spin_integrals(m,b,e,j);
					for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
						W_mbej(m,b,e,j) += t1(f,j)*spin_integrals(m,b,e,f);
					}
					for (int n = 0; n < p.n_occupied; ++n){
						W_mbej(m,b,e,j) += -t1(b,n)*spin_integrals(m,n,e,j);
						for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
							W_mbej(m,b,e,j) += -(0.5*t2(f,b,j,n) + t1(f,j)*t1(b,n))*spin_integrals(m,n,e,f);
						}
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::update_intermediates(){
	// We need to update our intermediates at the beginning, and
	// at the end of each iteration. Each iteration provides a new
	// guess at the amplitudes T1 (t1) and T2 (t2), that *hopefully*
	// converges to a stable, ground-state, solution.
	#ifdef timing
	if (mpi.rank==rank_master)	ccsd::Timer timer("update_intermediates",mpi.rank);
	#endif

	if (mpi.rank==F_ae.rank){get_Fae();}
	if (mpi.rank==F_mi.rank){get_Fmi();}
	if (mpi.rank==F_me.rank){get_Fme();}
	if (mpi.rank==W_mnij.rank){get_Wmnij();}
	if (mpi.rank==W_abef.rank){get_Wabef();}
	if (mpi.rank==W_mbej.rank){get_Wmbej();}
}
//=============================================================================


//=============================================================================

// makeT1 and makeT2, as they imply, construct the actual amplitudes necessary for computing
// the CCSD energy (or computing an EOM-CCSD Hamiltonian, etc)

// P13 collective-ops audit (pass-2):
// All sendVec2D / sendVec4D callsites in makeT1_s / makeT2_d are directed sends from a
// per-tensor compute rank (F_ae.rank, F_me.rank, F_mi.rank, W_abef.rank, W_mbej.rank, W_mnij.rank)
// to rank_master, which is the sole consumer inside the master-only loop nests below.
// The dataflow is scatter-compute -> gather-on-master -> broadcast-result: the resulting
// t1_next/t2_next are subsequently MPI_Bcast to all ranks in run(). Replacing the directed
// send with MPI_Bcast here would unnecessarily fan out per-tensor intermediates to every
// rank even though only rank_master uses them. Audit conclusion: keep point-to-point.

//-----------------------------------------------------------------------------
void CcsdSolver::sendVec2D(Vector2D &vec2D, int rank_src, int rank_dst){
	if (mpi.rank==rank_src){
			ccsd::mpi::send(vec2D, rank_dst);
	}
	if (mpi.rank==rank_dst){
			ccsd::mpi::recv(vec2D, rank_src);
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::sendVec4D(Vector4D &vec4D, int rank_src, int rank_dst){
	if (mpi.rank==rank_src){
			ccsd::mpi::send(vec4D, rank_dst);
	}
	if (mpi.rank==rank_dst){
			ccsd::mpi::recv(vec4D, rank_src);
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::makeT1_s(){ // Stanton eq (1)
	#ifdef timing
	if (mpi.rank==rank_master)	ccsd::Timer timer("makeT1_s",mpi.rank);
	#endif
	t1_next.zeros();

	if (mpi.size>1){
		sendVec2D(F_ae,F_ae.rank,rank_master);
		sendVec2D(F_me,F_me.rank,rank_master);
		sendVec2D(F_mi,F_mi.rank,rank_master);
	}

	if (mpi.rank==rank_master){
	CCSD_OMP_PARALLEL_FOR
	for (int a = p.n_occupied; a < n_spin_orbitals; ++a){
		for (int i = 0; i < p.n_occupied; ++i){
			t1_next(a,i) = fock_spin(i,a);                          // Stanton eq. (1), term 1: Fock off-diagonal
			for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
				t1_next(a,i) += t1(e,i)*F_ae(a,e);                  // term 2: T1·F_ae
			}
			for (int m = 0; m < p.n_occupied; ++m){
				t1_next(a,i) += -t1(a,m)* F_mi(m,i);               // term 3: T1·F_mi
				for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
					t1_next(a,i) += t2(a,e,i,m)* F_me(m,e);        // term 4: T2·F_me
					for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
						t1_next(a,i) += -0.5*t2(e,f,i,m)*spin_integrals(m,a,e,f);  // term 5: T2·<ma||ef>
					}
					for (int n = 0; n < p.n_occupied; ++n){
						t1_next(a,i) += -0.5*t2(a,e,m,n)*spin_integrals(n,m,e,i);  // term 6: T2·<nm||ei>
					}
				}
			}
			for (int n = 0; n < p.n_occupied; ++n){
				for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
					t1_next(a,i) += -t1(f,n)*spin_integrals(n,a,i,f);  // term 7: T1·<na||if>
				}
			}
			t1_next(a,i) = t1_next(a,i)/denom_ai(a,i);
		}
	}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::makeT2_d(){ // Stanton eq (2)
	#ifdef timing
	if (mpi.rank==rank_master)	ccsd::Timer timer("makeT2_d",mpi.rank);
	#endif
	t2_next.zeros();

	if (mpi.size>1){
		sendVec4D(W_abef,W_abef.rank,rank_master);
		sendVec4D(W_mbej,W_mbej.rank,rank_master);
		sendVec4D(W_mnij,W_mnij.rank,rank_master);
	}


	if (mpi.rank==rank_master){
		// Stanton eq. (2) — all terms accumulated into t2_next(a,b,i,j)
		CCSD_OMP_PARALLEL_FOR
		for (int a = p.n_occupied; a < n_spin_orbitals; ++a){
			for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
				for (int i = 0; i < p.n_occupied; ++i){
					for (int j = 0; j < p.n_occupied; ++j){
						t2_next(a,b,i,j) += spin_integrals(i,j,a,b);            // term 1: <ij||ab>
						for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
							t2_next(a,b,i,j) += t2(a,e,i,j)*F_ae(b,e)           // term 2a: T2·F_ae
												-t2(b,e,i,j)*F_ae(a,e);         // term 2b: antisymmetry
							for (int m = 0; m < p.n_occupied; ++m){
								t2_next(a,b,i,j) += -0.5*t2(a,e,i,j)*t1(b,m)* F_me(m,e)   // term 3a: T2·T1·F_me
													+ 0.5*t2(b,e,i,j)*t1(a,m)* F_me(m,e);  // term 3b: antisymmetry
							}
						}
						for (int m = 0; m < p.n_occupied; ++m){
							t2_next(a,b,i,j) += -t2(a,b,i,m)* F_mi(m,j)        // term 4a: T2·F_mi
												+ t2(a,b,j,m)* F_mi(m,i);      // term 4b: antisymmetry
							for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
								t2_next(a,b,i,j) += -0.5*t2(a,b,i,m)*t1(e,j)* F_me(m,e)   // term 5a: T2·T1·F_me
													+ 0.5*t2(a,b,j,m)*t1(e,i)* F_me(m,e);  // term 5b: antisymmetry
							}
						}
						for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
							t2_next(a,b,i,j) += t1(e,i)*spin_integrals(a,b,e,j) - t1(e,j)*spin_integrals(a,b,e,i);  // term 6: T1·<ab||ej>
							for (int f = p.n_occupied; f < n_spin_orbitals; ++f){
								t2_next(a,b,i,j) += 0.5*get_tau(e,f,i,j)*W_abef(a,b,e,f);  // term 7: tau·W_abef
							}
						}
						for (int m = 0; m < p.n_occupied; ++m){
							t2_next(a,b,i,j) += -t1(a,m)*spin_integrals(m,b,i,j) + t1(b,m)*spin_integrals(m,a,i,j);  // term 8: T1·<mb||ij>
							for (int e = p.n_occupied; e < n_spin_orbitals; ++e){
								t2_next(a,b,i,j) +=  t2(a,e,i,m)*W_mbej(m,b,e,j) - t1(e,i)*t1(a,m)*spin_integrals(m,b,e,j);  // term 9a: T2·W_mbej
								t2_next(a,b,i,j) += -t2(a,e,j,m)*W_mbej(m,b,e,i) + t1(e,j)*t1(a,m)*spin_integrals(m,b,e,i);  // term 9b
								t2_next(a,b,i,j) += -t2(b,e,i,m)*W_mbej(m,a,e,j) + t1(e,i)*t1(b,m)*spin_integrals(m,a,e,j);  // term 9c
								t2_next(a,b,i,j) +=  t2(b,e,j,m)*W_mbej(m,a,e,i) - t1(e,j)*t1(b,m)*spin_integrals(m,a,e,i);  // term 9d
							}
							for (int n = 0; n < p.n_occupied; ++n){
								t2_next(a,b,i,j) += 0.5*get_tau(a,b,m,n)*W_mnij(m,n,i,j);  // term 10: tau·W_mnij
							}
						}
						t2_next(a,b,i,j) /= denom_abij(a,b,i,j);
					}
				}
			}
		}
	}
}
//=============================================================================


//=============================================================================
double CcsdSolver::ccsdenergy() const { // Equation (134) and (173); Expression from Crawford, Schaefer (2000)
	// DOI: 10.1002/9780470125915.ch2
	// computes CCSD energy given T1 and T2
	double ECCSD = 0.0;
	for (int i = 0; i < p.n_occupied; ++i){
		for (int a = p.n_occupied; a < n_spin_orbitals; ++a){
			for (int j = 0; j < p.n_occupied; ++j){
				for (int b = p.n_occupied; b < n_spin_orbitals; ++b){
					ECCSD += 0.25*spin_integrals(i,j,a,b)*t2(a,b,i,j) + 0.5*spin_integrals(i,j,a,b)* t1(a,i) * t1(b,j);
				}
			}
		}
	}
	return ECCSD;
}
//=============================================================================




//=============================================================================
void CcsdSolver::run() {
	//================
	// MAIN LOOP
	// CCSD iteration
	//================
 	std::cout.precision(10);


	double cc_en = 0, cc_en_diff = 10.0;
	double cc_en_pre = 0;


	initialization();
	if (mpi.rank==rank_master){
		std::cout << "CCSD in MpiC++"<<std::endl;
	}


	while (cc_en_diff > ccsd::constants::convergence_threshold) {
		cc_en_pre = cc_en;
 		update_intermediates();
		makeT1_s();
		makeT2_d();

		ccsd::mpi::bcast(t2_next, rank_master);
		ccsd::mpi::bcast(t1_next, rank_master);

		t2=t2_next;
		t1=t1_next;

		if (mpi.rank==rank_master){
			cc_en = ccsdenergy();
			cc_en_diff = std::abs(cc_en - cc_en_pre);
		}

		//broadcast
		MPI_Bcast( &cc_en_diff, 1,  MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}


	if (mpi.rank==rank_master){
		std::cout<<"  E(corr,CCSD) = "<< cc_en<<std::endl;
		std::cout<<"  E(CCSD) = "<< cc_en + p.nuclear_repulsion + p.hf_energy<<std::endl;
	}
}
//=============================================================================
