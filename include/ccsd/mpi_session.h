#ifndef MpiClass_Included
#define MpiClass_Included

#include <mpi.h>

namespace ccsd {

class MpiSession {
public:
    MpiSession(int* argc, char*** argv) {
        MPI_Init(argc, argv);
        MPI_Comm_size(MPI_COMM_WORLD, &size_);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    }

    ~MpiSession() { MPI_Finalize(); }

    MpiSession(const MpiSession&) = delete;
    MpiSession& operator=(const MpiSession&) = delete;
    MpiSession(MpiSession&&) = delete;
    MpiSession& operator=(MpiSession&&) = delete;

    [[nodiscard]] int rank() const noexcept { return rank_; }
    [[nodiscard]] int size() const noexcept { return size_; }

private:
    int rank_ = 0;
    int size_ = 0;
};

}  // namespace ccsd

// Backwards-compat POD used by CcsdSolver for rank/size bookkeeping.
class MpiClass {
public:
    int size = 0;
    int rank = 0;
};

#endif  // MpiClass_Included
