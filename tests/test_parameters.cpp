#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <fstream>
#include <string>

#include <ccsd/parameters.h>

namespace {
const char* kFixture = R"({
  "dim": 2,
  "Nelec": 2,
  "orbital_energy": [-0.913, 1.395],
  "ENUC": 1.300,
  "EN": -2.854,
  "ttmo": [0.5, 0.31, 1.5, 0.42]
})";
}  // namespace

TEST_CASE("ParameterClass parses a name-keyed config", "[parameters]") {
    const std::string path = "test_config_tmp.json";
    {
        std::ofstream out(path);
        out << kFixture;
    }
    ParameterClass p(path);
    REQUIRE(p.n_spatial_orbitals == 2);
    REQUIRE(p.n_occupied == 2);
    REQUIRE(p.orbital_energies[0] == -0.913);
    REQUIRE(p.orbital_energies[1] ==  1.395);
    REQUIRE(p.nuclear_repulsion ==  1.300);
    REQUIRE(p.hf_energy   == -2.854);
    REQUIRE(p.two_electron_mos.at(0.5) == 0.31);
    REQUIRE(p.two_electron_mos.at(1.5) == 0.42);
    std::remove(path.c_str());
}
