#include <catch2/catch_test_macros.hpp>

#include <ccsd/ccsd_solver.h>

// get_key is a pure static function: compound index from 4 doubles.
// Formula produces a unique upper-triangular pair key such that
//   get_key(a,b,c,d) == get_key(b,a,c,d) == get_key(a,b,d,c) etc.

TEST_CASE("get_key is symmetric in first pair", "[math]") {
    REQUIRE(CcsdSolver::get_key(1.0, 2.0, 3.0, 4.0) ==
            CcsdSolver::get_key(2.0, 1.0, 3.0, 4.0));
}

TEST_CASE("get_key is symmetric in second pair", "[math]") {
    REQUIRE(CcsdSolver::get_key(1.0, 2.0, 3.0, 4.0) ==
            CcsdSolver::get_key(1.0, 2.0, 4.0, 3.0));
}

TEST_CASE("get_key diagonal returns zero-based single-index", "[math]") {
    // get_key(0,0,0,0): ab=0, cd=0, abcd=0
    REQUIRE(CcsdSolver::get_key(0.0, 0.0, 0.0, 0.0) == 0.0);
}

TEST_CASE("get_key distinct inputs produce distinct keys", "[math]") {
    double k1 = CcsdSolver::get_key(0.0, 1.0, 0.0, 1.0);
    double k2 = CcsdSolver::get_key(0.0, 1.0, 0.0, 2.0);
    double k3 = CcsdSolver::get_key(0.0, 2.0, 0.0, 2.0);
    REQUIRE(k1 != k2);
    REQUIRE(k2 != k3);
    REQUIRE(k1 != k3);
}
