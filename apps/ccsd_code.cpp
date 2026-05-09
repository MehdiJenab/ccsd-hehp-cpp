#include <ccsd/ccsd_solver.h>

int main(int argc, char** argv) {
    ccsd::MpiSession session(&argc, &argv);
    ccsd::CcsdSolver solver;
    solver.orchestrator.mpi.size = session.size();
    solver.orchestrator.mpi.rank = session.rank();
    solver.run();
    return 0;
}
