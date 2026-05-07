#include <ccsd/ccsd_solver.hpp>

int main(int argc, char** argv) {
    ccsd::MpiSession session(&argc, &argv);
    CcsdSolver solver;
    solver.mpi.size = session.size();
    solver.mpi.rank = session.rank();
    solver.run();
    return 0;
}
