

/* 
 |=============================================================================
 | CCSD in C++
 | Mehdi Jenab, mehdi.h.jenab@gmail.com
 |
 |
 | to install jansson.h, documentation is available at: 
 |		https://jansson.readthedocs.io/en/2.1/gettingstarted.html
 | 
 | to install OpenMPI:
 |		sudo apt install libopenmpi-dev
 |		sudo apt install openmpi-bin
 |
 | to compile the code: 
 | 		$ mpic++ ccsd_code.cpp -o ccsd_code_p -ljansson
 | to run the code: 
 |		$mpirun -n #number_cpu ccsd_code_p, 
 | #number_cpu= number of CPUs available for executing the code
 |==============================================================================  
*/


#include <cmath> 
#include <vector>
#include <map>
#include <chrono>  		// for timing C++11

#include <mpi.h> 		//parallel programming
#include <jansson.h>	// to handle JSON file 

#include "VectorsClass.h"
#include "ParameterClass.h" 
// #include "ParameterClass_NoJson.h" 
// in case, one wants to avoid using JSON file format use this instead of "ParameterClass.h"
#include "MpiClass.h"

// #define timing //for timing different functions


#define mLoop for(int m=0;m<p.Nelec;++m)
#define nLoop for(int n=0;n<p.Nelec;++n)


#define iLoop for(int i=0;i<p.Nelec;++i)
#define jLoop for(int j=0;j<p.Nelec;++j)

#define eLoop for(int e=p.Nelec;e<dim2;++e)
#define fLoop for(int f=p.Nelec;f<dim2;++f)

#define aLoop for(int a=p.Nelec;a<dim2;++a)
#define bLoop for(int b=p.Nelec;b<dim2;++b)


using namespace std;

ParameterClass  p;  // INITIALIZE ORBITAL ENERGIES //  AND TRANSFORMED TWO ELECTRON INTEGRALS  
MpiClass  mpi;

int rank_master, rank_start;

Vector2D Fae, Fmi, Fme, Dai, tsnew, ts, fs;
Vector4D Wmnij,Wabef,Wmbej,Dabij,tdnew,td,spinints;

int dim2; 


struct Timer
{
	std::chrono::time_point<std::chrono::system_clock> start,end;
	std::chrono::duration<float> duration;
	const char* name;
	int rank;
	Timer(const char* name_in, int & rank_in)
	{
		start = std::chrono::high_resolution_clock::now();
		name = name_in;
		rank =  rank_in;
	}
	
	~Timer()
	{
		end = std::chrono::high_resolution_clock::now();
		duration = end-start;
		float ms = duration.count() *  1000.0f;
		std::cout<< " timer:"<< ms <<" ms "<< "for "<< name<<"in rank="<<rank<<std::endl;
	}
	
};


//=============================================================================
	void guess_T2(){
		aLoop{
			bLoop{
				iLoop{
					jLoop{
						*td.set(a,b,i,j) += spinints(i,j,a,b)/(fs(i,i) + fs(j,j) - fs(a,a) - fs(b,b));
					}
				}
			}
		}
	}
//=============================================================================

//=============================================================================
	void get_denominator_arrays(){ // Make denominator arrays Dai, Dabij, Equation (12) of Stanton
		
		Dai.zeros();
		aLoop {
			iLoop{
				*Dai.set(a,i) = fs(i,i) - fs(a,a);
			}
		}
		Dabij.zeros();
		aLoop {
			bLoop{
				iLoop{
					jLoop{
						*Dabij.set(a,b,i,j) = fs(i,i) + fs(j,j) - fs(a,a) - fs(b,b);
					}
				}
			}
		}
	}
//=============================================================================


//=============================================================================
	double get_key(double &a,double &b,double &c,double &d){ // Return compound index given four indices		
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
	double get_value(double a,double b,double c,double d){ // Return Value of spatial MO two electron integral
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
	void get_spinints(){ // CONVERT SPATIAL TO SPIN ORBITAL MO,
		//This makes the spin basis double bar integral (physicists' notation)
		spinints.zeros();
		double value1,value2;
		for (size_t p = 1; p < dim2+1; ++p) {
			for (size_t q = 1; q < dim2+1; ++q) {
				for (size_t r = 1; r < dim2+1; ++r) {
					for (size_t s = 1; s < dim2+1; ++s) {
						value1 = get_value( (p+1)>>1,(r+1)>>1,(q+1)>>1,(s+1)>>1 ) * (p%2 == r%2) * (q%2 == s%2);
						value2 = get_value( (p+1)>>1,(s+1)>>1,(q+1)>>1,(r+1)>>1 ) * (p%2 == s%2) * (q%2 == r%2);
						*spinints.set(p-1,q-1,r-1,s-1) = value1 - value2;
					}
				}
			}
		}
	}
//=============================================================================


//=============================================================================
	void get_fs(){//Spin basis fock matrix eigenvalues, put MO energies in diagonal array
		vector<double> fs_1D;
		fs_1D.resize(dim2);
		fill(fs_1D.begin(), fs_1D.end(), 0.0);

		for (size_t i = 0; i < fs_1D.size(); ++i){
			fs_1D[i] = p.orbital_energy[floor(i/2)];
		}

		fs.diagonalize(fs_1D);

	}
//=============================================================================


//=============================================================================
	int update(int &i){
		i++ ;
		if (i>mpi.NODE){
			i=rank_start;
		}
		return i;
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	void get_ranks(){
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
	void initialization_vectors(){
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
	void initialization(){
		mpi.Initialize_MPI();
		#ifdef timing
 		if (mpi.rank==rank_master)	Timer timer("initialization",mpi.rank);
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
	double get_taus(int a,int b,int i,int j){// Stanton eq (9)	
		double taus = td(a,b,i,j) + 0.5*(ts(a,i)*ts(b,j) - ts(b,i)*ts(a,j));
	return taus;
	}
//=============================================================================  


//=============================================================================
	double get_tau(int a,int b,int i,int j){ // Stanton eq (10)
		double tau=td(a,b,i,j) + ts(a,i)*ts(b,j) - ts(b,i)*ts(a,j);
	return tau;
	}
//============================================================================= 


//=============================================================================
	void get_Fae(){ // Stanton eq (3)
		Fae.zeros();
		aLoop {
			eLoop{
				*Fae.set(a,e) = (1 - (a == e))*fs(a,e);
				mLoop{
					*Fae.set(a,e) += -0.5*fs(m,e)*ts(a,m);
					fLoop{
						*Fae.set(a,e) += ts(f,m)*spinints(m,a,f,e);
						nLoop{
							*Fae.set(a,e) += -0.5*get_taus(a,f,m,n)*spinints(m,n,e,f);
						}
					}
				}
			}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void get_Fmi(){ // Stanton eq (4)
		Fmi.zeros();
		mLoop {
			iLoop {
				*Fmi.set(m,i) = (1 - (m == i))*fs(m,i);
				eLoop{
					*Fmi.set(m,i) += 0.5*ts(e,i)*fs(m,e);
					nLoop{
						*Fmi.set(m,i) += ts(e,n)*spinints(m,n,i,e);
						fLoop{
							*Fmi.set(m,i) += 0.5*get_taus(e,f,i,n)*spinints(m,n,e,f);
						}
					}
				}
			}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void get_Fme(){  // Stanton eq (5)
		Fme.zeros();
		mLoop{
			eLoop{
				*Fme.set(m,e) = fs(m,e);
				nLoop{
					fLoop{
						*Fme.set(m,e) += ts(f,n)*spinints(m,n,e,f);
					}
				}
			}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void get_Wmnij(){// Stanton eq (6)
		Wmnij.zeros();
		mLoop{
			nLoop{
				iLoop{
					jLoop{
						*Wmnij.set(m,n,i,j) = spinints(m,n,i,j);
						eLoop{
							*Wmnij.set(m,n,i,j) +=  ts(e,j)*spinints(m,n,i,e) 
												-ts(e,i)*spinints(m,n,j,e);
							fLoop{
								*Wmnij.set(m,n,i,j) += 0.25*get_tau(e,f,i,j)*spinints(m,n,e,f);
							}
						}
					}
				}
			}
		}	
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	void get_Wabef(){ // Stanton eq (7)
		Wabef.zeros();
		aLoop{
			bLoop{
				eLoop{
					fLoop{
						*Wabef.set(a,b,e,f) = spinints(a,b,e,f);
						mLoop{
							*Wabef.set(a,b,e,f) += -ts(b,m)*spinints(a,m,e,f) 
												+ts(a,m)*spinints(b,m,e,f);
							nLoop{
								*Wabef.set(a,b,e,f) += 0.25*get_tau(a,b,m,n)*spinints(m,n,e,f);
							}
						}
					}
				}
			}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void get_Wmbej(){ // Stanton eq (8)
		Wmbej.zeros();
		mLoop{
			bLoop{
				eLoop{
					jLoop{
						*Wmbej.set(m,b,e,j) = spinints(m,b,e,j);
						fLoop{
							*Wmbej.set(m,b,e,j) += ts(f,j)*spinints(m,b,e,f);
						}
						nLoop{
							*Wmbej.set(m,b,e,j) += -ts(b,n)*spinints(m,n,e,j);
							fLoop{
								*Wmbej.set(m,b,e,j) += -(0.5*td(f,b,j,n) + ts(f,j)*ts(b,n))*spinints(m,n,e,f);
							}
						}
					}
				}
			}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void update_intermediates(){
		// We need to update our intermediates at the beginning, and 
		// at the end of each iteration. Each iteration provides a new
		// guess at the amplitudes T1 (ts) and T2 (td), that *hopefully*
		// converges to a stable, ground-state, solution.
		#ifdef timing
		if (mpi.rank==rank_master)	Timer timer("update_intermediates",mpi.rank);
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

//-----------------------------------------------------------------------------
	void sendVec2D(Vector2D &vec2D, int &rank_src, int &rank_dst){  
		if (mpi.rank==rank_src){
				vec2D.send(rank_dst);
		}
		if (mpi.rank==rank_dst){
				vec2D.recv(rank_src);
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void sendVec4D(Vector4D &vec4D, int &rank_src, int &rank_dst){  
		if (mpi.rank==rank_src){
				vec4D.send(rank_dst);
		}
		if (mpi.rank==rank_dst){
				vec4D.recv(rank_src);
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void makeT1_s(){ // Stanton eq (1)
		#ifdef timing
		if (mpi.rank==rank_master)	Timer timer("makeT1_s",mpi.rank);
		#endif
		tsnew.zeros();

		if (mpi.size>1){		
			sendVec2D(Fae,Fae.rank,rank_master);
			sendVec2D(Fme,Fme.rank,rank_master);
			sendVec2D(Fmi,Fmi.rank,rank_master);
		}

		if (mpi.rank==rank_master){
		aLoop{
			iLoop{
				*tsnew.set(a,i) = fs(i,a);
				eLoop{
					*tsnew.set(a,i) += ts(e,i)*Fae(a,e);
				}
				mLoop{
					*tsnew.set(a,i) += -ts(a,m)* Fmi(m,i);
					eLoop{
						*tsnew.set(a,i) += td(a,e,i,m)* Fme(m,e);
						fLoop{
							*tsnew.set(a,i) += -0.5*td(e,f,i,m)*spinints(m,a,e,f);
						}
						nLoop{
							*tsnew.set(a,i) += -0.5*td(a,e,m,n)*spinints(n,m,e,i);
						}
					}
				}
				nLoop{
					fLoop{ 
						*tsnew.set(a,i) += -ts(f,n)*spinints(n,a,i,f);
					}
				}
				*tsnew.set(a,i) = tsnew(a,i)/Dai(a,i);
			}
		}
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	void makeT2_d(){ // Stanton eq (2)
		#ifdef timing
		if (mpi.rank==rank_master)	Timer timer("makeT2_d",mpi.rank);
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
			aLoop{
				bLoop{
					iLoop{
						jLoop{
							*tdnew.set(a,b,i,j) += spinints(i,j,a,b);
							eLoop{
								*tdnew.set(a,b,i,j) += td(a,e,i,j)*Fae(b,e)
													-td(b,e,i,j)*Fae(a,e);
								mLoop{
									*tdnew.set(a,b,i,j) += -0.5*td(a,e,i,j)*ts(b,m)* Fme(m,e) 
														+ 0.5*td(b,e,i,j)*ts(a,m)* Fme(m,e);
								}
							}
							mLoop{
								*tdnew.set(a,b,i,j) += -td(a,b,i,m)* Fmi(m,j)
													+ td(a,b,j,m)* Fmi(m,i);
								eLoop{
									*tdnew.set(a,b,i,j) += -0.5*td(a,b,i,m)*ts(e,j)* Fme(m,e) 
														+ 0.5*td(a,b,j,m)*ts(e,i)* Fme(m,e);
								}
							}
							eLoop{
								*tdnew.set(a,b,i,j) += ts(e,i)*spinints(a,b,e,j) - ts(e,j)*spinints(a,b,e,i);
								fLoop{
									*tdnew.set(a,b,i,j) += 0.5*get_tau(e,f,i,j)*Wabef(a,b,e,f);
								}
							}
							mLoop{
								*tdnew.set(a,b,i,j) += -ts(a,m)*spinints(m,b,i,j) + ts(b,m)*spinints(m,a,i,j);
								eLoop{
									*tdnew.set(a,b,i,j) +=  td(a,e,i,m)*Wmbej(m,b,e,j) - ts(e,i)*ts(a,m)*spinints(m,b,e,j);
									*tdnew.set(a,b,i,j) += -td(a,e,j,m)*Wmbej(m,b,e,i) + ts(e,j)*ts(a,m)*spinints(m,b,e,i);
									*tdnew.set(a,b,i,j) += -td(b,e,i,m)*Wmbej(m,a,e,j) + ts(e,i)*ts(b,m)*spinints(m,a,e,j);
									*tdnew.set(a,b,i,j) +=  td(b,e,j,m)*Wmbej(m,a,e,i) - ts(e,j)*ts(b,m)*spinints(m,a,e,i);
								}
								nLoop{
									*tdnew.set(a,b,i,j) += 0.5*get_tau(a,b,m,n)*Wmnij(m,n,i,j);
								}
							}
							*tdnew.set(a,b,i,j) /= Dabij(a,b,i,j);
						}
					}
				}
			}
		}
	}
//=============================================================================


//=============================================================================
	double  ccsdenergy(){ 	// Equation (134) and (173); Expression from Crawford, Schaefer (2000) 
		// DOI: 10.1002/9780470125915.ch2
		// computes CCSD energy given T1 and T2
		double ECCSD = 0.0;
		iLoop{		
			aLoop{
				jLoop{
					bLoop{
						ECCSD += 0.25*spinints(i,j,a,b)*td(a,b,i,j) + 0.5*spinints(i,j,a,b)* ts(a,i) * ts(b,j);
					}
				}
			}
		}
		return ECCSD;
	}
//=============================================================================    
    



//============================================================================= 
int main() {

	//================
	// MAIN LOOP
	// CCSD iteration
	//================
 	std::cout.precision(10);

	
	double cc_en = 0, cc_en_diff = 10.0;
	double cc_en_pre = 0;


	initialization();
	if (mpi.rank==rank_master){
		cout << "CCSD in MpiC++"<<endl;
	}


	while (cc_en_diff>10E-9) { // arbitrary convergence criteria
		cc_en_pre = cc_en;
 		update_intermediates();
		makeT1_s();
		makeT2_d();
		
		tdnew.mpi_bcast(rank_master);
		tsnew.mpi_bcast(rank_master);
		
		td=tdnew;
		ts=tsnew;

		if (mpi.rank==rank_master){
			cc_en = ccsdenergy();
			cc_en_diff = abs(cc_en - cc_en_pre);
		}
		
		//broadcast
		MPI_Bcast( &cc_en_diff, 1,  MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}


	if (mpi.rank==rank_master){
		cout<<"  E(corr,CCSD) = "<< cc_en<<endl;
		cout<<"  E(CCSD) = "<< cc_en + p.ENUC + p.EN<<endl;
	}


	mpi.Finalize_MPI();
    return 0;
}
//============================================================================= 
