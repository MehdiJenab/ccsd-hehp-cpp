#ifndef VectorsClass_Included
#define VectorsClas_Included



//=============================================================================
class Vector2D{
	int n1, n2;
	int n_size;
	std::vector<double> vec2D;

//-----------------------------------------------------------------------------
	int ij_to_index(int i1, int i2){
			return (i2 * n1) + i1;
	}
//-----------------------------------------------------------------------------

public:
	int rank=0;
//-----------------------------------------------------------------------------
	void initialization (int & dim2){
		n1=dim2;
		n2=dim2;
		n_size=n1*n2;
		vec2D.resize(n_size);
		fill(vec2D.begin(), vec2D.end(), 0.0);
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	void send(int &rank_dst){
		MPI_Send(&vec2D[0], n_size,  MPI_DOUBLE, rank_dst, 233, MPI_COMM_WORLD);        
    }
//-----------------------------------------------------------------------------
    void recv(int &rank_src){
		MPI_Recv(&vec2D[0], n_size,  MPI_DOUBLE, rank_src, 233, MPI_COMM_WORLD, MPI_STATUS_IGNORE);        
    }
//-----------------------------------------------------------------------------
	void mpi_bcast(int &rank_src){
		MPI_Bcast( &vec2D[0], n_size,  MPI_DOUBLE, rank_src, MPI_COMM_WORLD);
	}
//-----------------------------------------------------------------------------    

//-----------------------------------------------------------------------------
	void zeros(){
		fill(vec2D.begin(), vec2D.end(), 0.0);
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	void diagonalize(std::vector<double> &fs_1D){
		for (size_t i = 0; i < n1; ++i){
			for (size_t j = 0; j < n2; ++j){
				if (i==j){
					*this->set(i,j)=fs_1D[i];
				}else{
					*this->set(i,j)==0;
				}
			}
		}
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	Vector2D& operator=(const Vector2D &rhs) {
		if (this != &rhs){      //  to avoid self-assignment
			for (int i=0; i<n_size; ++i){
				vec2D[i]=rhs.vec2D[i];
			}
		}
		return *this; // Return a reference to myself.
	} 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	friend std::ostream& operator<<(std::ostream& os, Vector2D &v){
		int index;
		os<<"["<<std::endl;
		for(std::size_t i = 0; i < v.n1; ++i) {
			os<<"  [";
			for(std::size_t j = 0; j < v.n2; ++j) {
 				index = v.ij_to_index(i,j);
				os<<v.vec2D[index]<<",    ";
			}
			os<<"]"<<std::endl;
		}
		os<<"]"<<std::endl<<std::endl;
//  		for(std::size_t i = 0; i < v.vec2D.size(); ++i) {
//  			std::cout << "("<<i<<","<<v.vec2D[i] << ") ";	
//  		}

// 		os<<std::endl<<" ===== "<<std::endl;
		return os;
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------	
	double get_index( int i, int j ){
		if (i>n1 or j>n2){
			std::cout<< "error, out of range";
			return 0;
		}else{
			return vec2D[ij_to_index(i,j)];
		}
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	double operator()( int i, int j ){
		if (i>n1 or j>n2){
			std::cout<< "error, out of range";
			return 0;
		}else{
			return vec2D[ij_to_index(i,j)];
		}
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	double* set( int i, int j ){
		if (i>n1 or j>n2){
			std::cout<< "error, out of range";
			return NULL;
		}else{
			return &vec2D[ij_to_index(i,j)];
		}
	}
//-----------------------------------------------------------------------------
  
};
//=============================================================================


//=============================================================================
class Vector4D{

	int n1, n2, n3, n4;
	int n_size;
	std::vector<double> vec4D;

//-----------------------------------------------------------------------------
	int ijkl_to_index(int i1, int i2, int i3, int i4) {
		int index=0;
		index += i4*(n1*n2*n3);
		index += i3*(n1*n2);
		index += i2*(n1);
		index += i1;
		return index;
	}
//-----------------------------------------------------------------------------


public:
	int rank=0;
//-----------------------------------------------------------------------------
	void initialization (int & dim2){
		n1=dim2;
		n2=dim2;
		n3=dim2;
		n4=dim2;
		n_size=n1*n2*n3*n4;
		vec4D.resize(n_size);
		fill(vec4D.begin(), vec4D.end(), 0.0);
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	void send(int &rank_dst){
		MPI_Send(&vec4D[0], n_size,  MPI_DOUBLE, rank_dst, 610, MPI_COMM_WORLD);        
    }
//-----------------------------------------------------------------------------
    void recv(int &rank_src){
		MPI_Recv(&vec4D[0], n_size,  MPI_DOUBLE, rank_src, 610, MPI_COMM_WORLD, MPI_STATUS_IGNORE);        
    }
//-----------------------------------------------------------------------------
	void zeros(){
		fill(vec4D.begin(), vec4D.end(), 0.0);
	}
//-----------------------------------------------------------------------------
	void mpi_bcast(int &rank_src){
		MPI_Bcast( &vec4D[0], n_size,  MPI_DOUBLE, rank_src, MPI_COMM_WORLD);
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------	
	Vector4D& operator=(const Vector4D &rhs) {
		if (this != &rhs){      //  to avoid self-assignment
			for (int i=0; i<n_size; ++i){
				vec4D[i]=rhs.vec4D[i];
			}
		}
		return *this; // Return a reference to myself.
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	friend std::ostream& operator<<(std::ostream& os, Vector4D &v){
		int index;
		for(std::size_t i = 0; i < v.n1; ++i) {
			os<<"["<<std::endl;
			for(std::size_t j = 0; j < v.n2; ++j) {
				os<<"   ["<<std::endl;
				for(std::size_t k = 0; k < v.n3; ++k) {
					os<<"      [";
					for(std::size_t l = 0; l < v.n4; ++l) {
						index = v.ijkl_to_index(i,j,k,l);
						os<<v.vec4D[index]<<",    ";
					}
					os<<"]"<<std::endl;
				}
				os<<"   ]"<<std::endl;
			}
			os<<"]"<<std::endl<<std::endl;
		}
//  		for(std::size_t i = 0; i < v.vec4D.size(); ++i) {
//  			std::cout << "("<<i<<","<<v.vec4D[i] << ") ";	
//  		}

//  		for(std::vector<double>::iterator itr = v.vec4D.begin(); itr != v.vec4D.end(); ++itr) {
//  			os << *itr << "\n";
//  		}
// 		os<<std::endl<<"======"<<std::endl;
		return os;
	}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
	double operator()( int i, int j, int k, int l ){
		if (i>n1 or j>n2 or k>n3 or l>n4){
			std::cout<< "error, out of range";
			return 0;
		}else{
			return vec4D[ ijkl_to_index(i,j,k,l) ];
		}
	}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
	double* set( int i, int j, int k, int l ){
		if (i>n1 or j>n2 or k>n3 or l>n4){
			std::cout<< "error, out of range";
			return NULL;
		}else{
			return &vec4D[ ijkl_to_index(i,j,k,l) ];
		}
	}
//-----------------------------------------------------------------------------
};

#endif //VectorsClass_Included
