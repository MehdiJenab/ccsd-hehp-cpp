#ifndef ParameterClass_Included
#define ParameterClass_Included

#include <array>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

class ParameterClass {
public:
    int dim = 0;
    int Nelec = 0;
    std::array<double, 2> orbital_energy{};
    double ENUC = 0.0;
    double EN = 0.0;
    std::map<double, double> ttmo;

    explicit ParameterClass(const std::string& path = "./config.json") {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open " + path);
        nlohmann::json j;
        in >> j;

        dim   = j.at("dim").get<int>();
        Nelec = j.at("Nelec").get<int>();

        const auto& oe = j.at("orbital_energy");
        if (oe.size() != 2) throw std::runtime_error("orbital_energy must have 2 entries");
        orbital_energy[0] = oe[0].get<double>();
        orbital_energy[1] = oe[1].get<double>();

        ENUC = j.at("ENUC").get<double>();
        EN   = j.at("EN").get<double>();

        const auto& flat = j.at("ttmo");
        for (std::size_t i = 0; i + 1 < flat.size(); i += 2) {
            ttmo.emplace(flat[i].get<double>(), flat[i + 1].get<double>());
        }
    }
};

#endif  // ParameterClass_Included
