#include <ccsd/ccsd_solver.h>
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

//=============================================================================
void CcsdSolver::guess_T2(){
	for (int a = p.Nelec; a < dim2; ++a){
		for (int b = p.Nelec; b < dim2; ++b){
			for (int i = 0; i < p.Nelec; ++i){
				for (int j = 0; j < p.Nelec; ++j){
					td(a,b,i,j) += spinints(i,j,a,b)/(fs(i,i) + fs(j,j) - fs(a,a) - fs(b,b));
				}
			}
		}
	}
}
//=============================================================================

//=============================================================================
void CcsdSolver::get_denominator_arrays(){ // Make denominator arrays Dai, Dabij, Equation (12) of Stanton

	Dai.zeros();
	for (int a = p.Nelec; a < dim2; ++a) {
		for (int i = 0; i < p.Nelec; ++i){
			Dai(a,i) = fs(i,i) - fs(a,a);
		}
	}
	Dabij.zeros();
	for (int a = p.Nelec; a < dim2; ++a) {
		for (int b = p.Nelec; b < dim2; ++b){
			for (int i = 0; i < p.Nelec; ++i){
				for (int j = 0; j < p.Nelec; ++j){
					Dabij(a,b,i,j) = fs(i,i) + fs(j,j) - fs(a,a) - fs(b,b);
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
	if ( p.ttmo.find(key) == p.ttmo.end() ) {
		value=0.0e0;
	} else {
		value = p.ttmo.find(key)->second;
	}
	return  value;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CcsdSolver::get_spinints(){ // CONVERT SPATIAL TO SPIN ORBITAL MO,
	//This makes the spin basis double bar integral (physicists' notation)
	spinints.zeros();
	double value1,value2;
	for (int pp = 1; pp < dim2 + 1; ++pp) {
		for (int qq = 1; qq < dim2 + 1; ++qq) {
			for (int rr = 1; rr < dim2 + 1; ++rr) {
				for (int ss = 1; ss < dim2 + 1; ++ss) {
					value1 = get_value( static_cast<double>((pp+1)>>1), static_cast<double>((rr+1)>>1), static_cast<double>((qq+1)>>1), static_cast<double>((ss+1)>>1) ) * (pp%2 == rr%2) * (qq%2 == ss%2);
					value2 = get_value( static_cast<double>((pp+1)>>1), static_cast<double>((ss+1)>>1), static_cast<double>((qq+1)>>1), static_cast<double>((rr+1)>>1) ) * (pp%2 == ss%2) * (qq%2 == rr%2);
					spinints(pp-1, qq-1, rr-1, ss-1) = value1 - value2;
				}
			}
		}
	}
}
//=============================================================================


//=============================================================================
void CcsdSolver::get_fs(){//Spin basis fock matrix eigenvalues, put MO energies in diagonal array
	vector<double> fs_1D;
	fs_1D.resize(static_cast<std::size_t>(dim2));
	fill(fs_1D.begin(), fs_1D.end(), 0.0);

	for (std::size_t i = 0; i < fs_1D.size(); ++i){
		fs_1D[i] = p.orbital_energy[i / 2];
	}

	fs.diagonalize(fs_1D);

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
	Fae.rank = i;
	i = update(i);
	Fme.rank = i;
	i = update(i);
	Fmi.rank = i;
	i = update(i);
	Wmbej.rank = i;
	i = update(i);
	Wmnij.rank = i;
	i = update(i);
	Wabef.rank = i;
}
//=============================================================================

//-----------------------------------------------------------------------------
void CcsdSolver::initialization_vectors(){
	Fae.initialization(dim2);
	Fmi.initialization(dim2);
	Fme.initialization(dim2);
	Dai.initialization(dim2);
	tsnew.initialization(dim2);
	ts.initialization(dim2);
	fs.initialization(dim2);

	Wmnij.initialization(dim2);
	Wabef.initialization(dim2);
	Wmbej.initialization(dim2);
	Dabij.initialization(dim2);
	tdnew.initialization(dim2);
	td.initialization(dim2);
	spinints.initialization(dim2);;
}
//-----------------------------------------------------------------------------

//=============================================================================
void CcsdSolver::initialization(){
	// MPI lifetime is now owned by ccsd::MpiSession in main().
	#ifdef timing
 	if (mpi.rank==rank_master)	ccsd::Timer timer("initialization",mpi.rank);
	#endif

	dim2= 2*p.dim;

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
	double taus = td(a,b,i,j) + 0.5*(ts(a,i)*ts(b,j) - ts(b,i)*ts(a,j));
	return taus;
}
//=============================================================================


//=============================================================================
double CcsdSolver::get_tau(int a, int b, int i, int j) const { // Stanton eq (10)
	double tau=td(a,b,i,j) + ts(a,i)*ts(b,j) - ts(b,i)*ts(a,j);
	return tau;
}
//=============================================================================


//=============================================================================
void CcsdSolver::get_Fae(){ // Stanton eq (3)
	Fae.zeros();
	for (int a = p.Nelec; a < dim2; ++a) {
		for (int e = p.Nelec; e < dim2; ++e){
			Fae(a,e) = (1 - (a == e))*fs(a,e);
			for (int m = 0; m < p.Nelec; ++m){
				Fae(a,e) += -0.5*fs(m,e)*ts(a,m);
				for (int f = p.Nelec; f < dim2; ++f){
					Fae(a,e) += ts(f,m)*spinints(m,a,f,e);
					for (int n = 0; n < p.Nelec; ++n){
						Fae(a,e) += -0.5*get_taus(a,f,m,n)*spinints(m,n,e,f);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Fmi(){ // Stanton eq (4)
	Fmi.zeros();
	for (int m = 0; m < p.Nelec; ++m) {
		for (int i = 0; i < p.Nelec; ++i) {
			Fmi(m,i) = (1 - (m == i))*fs(m,i);
			for (int e = p.Nelec; e < dim2; ++e){
				Fmi(m,i) += 0.5*ts(e,i)*fs(m,e);
				for (int n = 0; n < p.Nelec; ++n){
					Fmi(m,i) += ts(e,n)*spinints(m,n,i,e);
					for (int f = p.Nelec; f < dim2; ++f){
						Fmi(m,i) += 0.5*get_taus(e,f,i,n)*spinints(m,n,e,f);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Fme(){  // Stanton eq (5)
	Fme.zeros();
	for (int m = 0; m < p.Nelec; ++m){
		for (int e = p.Nelec; e < dim2; ++e){
			Fme(m,e) = fs(m,e);
			for (int n = 0; n < p.Nelec; ++n){
				for (int f = p.Nelec; f < dim2; ++f){
					Fme(m,e) += ts(f,n)*spinints(m,n,e,f);
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CcsdSolver::get_Wmnij(){// Stanton eq (6)
	Wmnij.zeros();
	for (int m = 0; m < p.Nelec; ++m){
		for (int n = 0; n < p.Nelec; ++n){
			for (int i = 0; i < p.Nelec; ++i){
				for (int j = 0; j < p.Nelec; ++j){
					Wmnij(m,n,i,j) = spinints(m,n,i,j);
					for (int e = p.Nelec; e < dim2; ++e){
						Wmnij(m,n,i,j) +=  ts(e,j)*spinints(m,n,i,e)
											-ts(e,i)*spinints(m,n,j,e);
						for (int f = p.Nelec; f < dim2; ++f){
							Wmnij(m,n,i,j) += 0.25*get_tau(e,f,i,j)*spinints(m,n,e,f);
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
	Wabef.zeros();
	for (int a = p.Nelec; a < dim2; ++a){
		for (int b = p.Nelec; b < dim2; ++b){
			for (int e = p.Nelec; e < dim2; ++e){
				for (int f = p.Nelec; f < dim2; ++f){
					Wabef(a,b,e,f) = spinints(a,b,e,f);
					for (int m = 0; m < p.Nelec; ++m){
						Wabef(a,b,e,f) += -ts(b,m)*spinints(a,m,e,f)
											+ts(a,m)*spinints(b,m,e,f);
						for (int n = 0; n < p.Nelec; ++n){
							Wabef(a,b,e,f) += 0.25*get_tau(a,b,m,n)*spinints(m,n,e,f);
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
	Wmbej.zeros();
	CCSD_OMP_PARALLEL_FOR
	for (int m = 0; m < p.Nelec; ++m){
		for (int b = p.Nelec; b < dim2; ++b){
			for (int e = p.Nelec; e < dim2; ++e){
				for (int j = 0; j < p.Nelec; ++j){
					Wmbej(m,b,e,j) = spinints(m,b,e,j);
					for (int f = p.Nelec; f < dim2; ++f){
						Wmbej(m,b,e,j) += ts(f,j)*spinints(m,b,e,f);
					}
					for (int n = 0; n < p.Nelec; ++n){
						Wmbej(m,b,e,j) += -ts(b,n)*spinints(m,n,e,j);
						for (int f = p.Nelec; f < dim2; ++f){
							Wmbej(m,b,e,j) += -(0.5*td(f,b,j,n) + ts(f,j)*ts(b,n))*spinints(m,n,e,f);
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
	// guess at the amplitudes T1 (ts) and T2 (td), that *hopefully*
	// converges to a stable, ground-state, solution.
	#ifdef timing
	if (mpi.rank==rank_master)	ccsd::Timer timer("update_intermediates",mpi.rank);
	#endif

	if (mpi.rank==Fae.rank){get_Fae();}
	if (mpi.rank==Fmi.rank){get_Fmi();}
	if (mpi.rank==Fme.rank){get_Fme();}
	if (mpi.rank==Wmnij.rank){get_Wmnij();}
	if (mpi.rank==Wabef.rank){get_Wabef();}
	if (mpi.rank==Wmbej.rank){get_Wmbej();}
}
//=============================================================================


//=============================================================================

// makeT1 and makeT2, as they imply, construct the actual amplitudes necessary for computing
// the CCSD energy (or computing an EOM-CCSD Hamiltonian, etc)

// P13 collective-ops audit (pass-2):
// All sendVec2D / sendVec4D callsites in makeT1_s / makeT2_d are directed sends from a
// per-tensor compute rank (Fae.rank, Fme.rank, Fmi.rank, Wabef.rank, Wmbej.rank, Wmnij.rank)
// to rank_master, which is the sole consumer inside the master-only loop nests below.
// The dataflow is scatter-compute -> gather-on-master -> broadcast-result: the resulting
// tsnew/tdnew are subsequently MPI_Bcast to all ranks in run(). Replacing the directed
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
	tsnew.zeros();

	if (mpi.size>1){
		sendVec2D(Fae,Fae.rank,rank_master);
		sendVec2D(Fme,Fme.rank,rank_master);
		sendVec2D(Fmi,Fmi.rank,rank_master);
	}

	if (mpi.rank==rank_master){
	CCSD_OMP_PARALLEL_FOR
	for (int a = p.Nelec; a < dim2; ++a){
		for (int i = 0; i < p.Nelec; ++i){
			tsnew(a,i) = fs(i,a);
			for (int e = p.Nelec; e < dim2; ++e){
				tsnew(a,i) += ts(e,i)*Fae(a,e);
			}
			for (int m = 0; m < p.Nelec; ++m){
				tsnew(a,i) += -ts(a,m)* Fmi(m,i);
				for (int e = p.Nelec; e < dim2; ++e){
					tsnew(a,i) += td(a,e,i,m)* Fme(m,e);
					for (int f = p.Nelec; f < dim2; ++f){
						tsnew(a,i) += -0.5*td(e,f,i,m)*spinints(m,a,e,f);
					}
					for (int n = 0; n < p.Nelec; ++n){
						tsnew(a,i) += -0.5*td(a,e,m,n)*spinints(n,m,e,i);
					}
				}
			}
			for (int n = 0; n < p.Nelec; ++n){
				for (int f = p.Nelec; f < dim2; ++f){
					tsnew(a,i) += -ts(f,n)*spinints(n,a,i,f);
				}
			}
			tsnew(a,i) = tsnew(a,i)/Dai(a,i);
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
	tdnew.zeros();

	if (mpi.size>1){
	// 	sendVec2D(Fae,Fae.rank,rank_master);
	// 	sendVec2D(Fme,Fme.rank,rank_master);
	// 	sendVec2D(Fmi,Fmi.rank,rank_master);
		sendVec4D(Wabef,Wabef.rank,rank_master);
		sendVec4D(Wmbej,Wmbej.rank,rank_master);
		sendVec4D(Wmnij,Wmnij.rank,rank_master);
	}


	if (mpi.rank==rank_master){
		CCSD_OMP_PARALLEL_FOR
		for (int a = p.Nelec; a < dim2; ++a){
			for (int b = p.Nelec; b < dim2; ++b){
				for (int i = 0; i < p.Nelec; ++i){
					for (int j = 0; j < p.Nelec; ++j){
						tdnew(a,b,i,j) += spinints(i,j,a,b);
						for (int e = p.Nelec; e < dim2; ++e){
							tdnew(a,b,i,j) += td(a,e,i,j)*Fae(b,e)
												-td(b,e,i,j)*Fae(a,e);
							for (int m = 0; m < p.Nelec; ++m){
								tdnew(a,b,i,j) += -0.5*td(a,e,i,j)*ts(b,m)* Fme(m,e)
													+ 0.5*td(b,e,i,j)*ts(a,m)* Fme(m,e);
							}
						}
						for (int m = 0; m < p.Nelec; ++m){
							tdnew(a,b,i,j) += -td(a,b,i,m)* Fmi(m,j)
												+ td(a,b,j,m)* Fmi(m,i);
							for (int e = p.Nelec; e < dim2; ++e){
								tdnew(a,b,i,j) += -0.5*td(a,b,i,m)*ts(e,j)* Fme(m,e)
													+ 0.5*td(a,b,j,m)*ts(e,i)* Fme(m,e);
							}
						}
						for (int e = p.Nelec; e < dim2; ++e){
							tdnew(a,b,i,j) += ts(e,i)*spinints(a,b,e,j) - ts(e,j)*spinints(a,b,e,i);
							for (int f = p.Nelec; f < dim2; ++f){
								tdnew(a,b,i,j) += 0.5*get_tau(e,f,i,j)*Wabef(a,b,e,f);
							}
						}
						for (int m = 0; m < p.Nelec; ++m){
							tdnew(a,b,i,j) += -ts(a,m)*spinints(m,b,i,j) + ts(b,m)*spinints(m,a,i,j);
							for (int e = p.Nelec; e < dim2; ++e){
								tdnew(a,b,i,j) +=  td(a,e,i,m)*Wmbej(m,b,e,j) - ts(e,i)*ts(a,m)*spinints(m,b,e,j);
								tdnew(a,b,i,j) += -td(a,e,j,m)*Wmbej(m,b,e,i) + ts(e,j)*ts(a,m)*spinints(m,b,e,i);
								tdnew(a,b,i,j) += -td(b,e,i,m)*Wmbej(m,a,e,j) + ts(e,i)*ts(b,m)*spinints(m,a,e,j);
								tdnew(a,b,i,j) +=  td(b,e,j,m)*Wmbej(m,a,e,i) - ts(e,j)*ts(b,m)*spinints(m,a,e,i);
							}
							for (int n = 0; n < p.Nelec; ++n){
								tdnew(a,b,i,j) += 0.5*get_tau(a,b,m,n)*Wmnij(m,n,i,j);
							}
						}
						tdnew(a,b,i,j) /= Dabij(a,b,i,j);
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
	for (int i = 0; i < p.Nelec; ++i){
		for (int a = p.Nelec; a < dim2; ++a){
			for (int j = 0; j < p.Nelec; ++j){
				for (int b = p.Nelec; b < dim2; ++b){
					ECCSD += 0.25*spinints(i,j,a,b)*td(a,b,i,j) + 0.5*spinints(i,j,a,b)* ts(a,i) * ts(b,j);
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


	while (cc_en_diff>10E-9) { // arbitrary convergence criteria
		cc_en_pre = cc_en;
 		update_intermediates();
		makeT1_s();
		makeT2_d();

		ccsd::mpi::bcast(tdnew, rank_master);
		ccsd::mpi::bcast(tsnew, rank_master);

		td=tdnew;
		ts=tsnew;

		if (mpi.rank==rank_master){
			cc_en = ccsdenergy();
			cc_en_diff = std::abs(cc_en - cc_en_pre);
		}

		//broadcast
		MPI_Bcast( &cc_en_diff, 1,  MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}


	if (mpi.rank==rank_master){
		std::cout<<"  E(corr,CCSD) = "<< cc_en<<std::endl;
		std::cout<<"  E(CCSD) = "<< cc_en + p.ENUC + p.EN<<std::endl;
	}
}
//=============================================================================
