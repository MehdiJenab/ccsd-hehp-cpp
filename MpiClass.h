#ifndef MpiClass_Included
#define MpiClass_Included


class MpiClass {

public:
	int size, rank,NODE;
	MPI_Comm  COMM_CART;

//=============================================================================
	void Initialize_MPI(){	
		// Initialize the MPI environment
		MPI_Init(NULL, NULL);

		// Get the number of processes
		MPI_Comm_size(MPI_COMM_WORLD, &size);

		// Get the rank of the process
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	}
//============================================================================= 
	
//=============================================================================
	void Finalize_MPI(){
		 MPI_Finalize();
	} 
//=============================================================================
};

#endif // MpiClass_Included
