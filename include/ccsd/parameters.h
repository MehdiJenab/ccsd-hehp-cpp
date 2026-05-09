#ifndef ParameterClass_Included
#define ParameterClass_Included

#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class ParameterClass {
public:
    int n_spatial_orbitals = 0;
    int n_occupied = 0;
    std::vector<double> orbital_energies;
    double nuclear_repulsion = 0.0;
    double hf_energy = 0.0;
    std::map<double, double> two_electron_mos;

    explicit ParameterClass(const std::string& path = "./config.json") {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open " + path);
        nlohmann::json j;
        in >> j;

        n_spatial_orbitals = j.at("dim").get<int>();
        n_occupied         = j.at("Nelec").get<int>();

        const auto& oe = j.at("orbital_energy");
        orbital_energies.resize(oe.size());
        for (std::size_t i = 0; i < oe.size(); ++i)
            orbital_energies[i] = oe[i].get<double>();

        nuclear_repulsion = j.at("ENUC").get<double>();
        hf_energy         = j.at("EN").get<double>();

        const auto& flat = j.at("ttmo");
        for (std::size_t i = 0; i + 1 < flat.size(); i += 2) {
            two_electron_mos.emplace(flat[i].get<double>(), flat[i + 1].get<double>());
        }

        validate();
    }

    void validate() const {
        if (n_occupied > 2 * n_spatial_orbitals)
            throw std::runtime_error("n_occupied exceeds 2 * n_spatial_orbitals");
        if (static_cast<int>(orbital_energies.size()) != n_spatial_orbitals)
            throw std::runtime_error("orbital_energies size mismatch");
    }
};

#endif  // ParameterClass_Included
