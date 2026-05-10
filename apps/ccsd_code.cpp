#include <solver/ccsd_solver.h>

int main(int argc, char** argv) {
    ccsd::MpiSession session(&argc, &argv);
    ccsd::CcsdSolver solver;
    solver.attach(session);
    solver.run();
    return 0;
}
