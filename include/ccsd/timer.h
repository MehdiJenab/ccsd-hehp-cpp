#ifndef CCSD_TIMER_H
#define CCSD_TIMER_H

#include <chrono>
#include <cstdio>
#include <string_view>

namespace ccsd {

struct Timer {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point start;
    std::string_view name;
    int rank;

    Timer(std::string_view name_in, int rank_in)
        : start(clock::now()), name(name_in), rank(rank_in) {}

    ~Timer() {
        auto end = clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        std::printf(" timer: %.1f ms for %.*s in rank=%d\n",
                    ms, static_cast<int>(name.size()), name.data(), rank);
    }
};

}  // namespace ccsd

#endif  // CCSD_TIMER_H
