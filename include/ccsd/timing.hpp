#ifndef CCSD_TIMING_HPP
#define CCSD_TIMING_HPP

#include <algorithm>
#include <chrono>
#include <vector>

namespace ccsd::timing {

class PercentileAccumulator {
public:
    using clock      = std::chrono::steady_clock;
    using time_point = clock::time_point;

    void start() { t0_ = clock::now(); }

    void stop() {
        auto t1 = clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0_).count();
        samples_.push_back(static_cast<double>(us));
    }

    [[nodiscard]] std::size_t count() const noexcept { return samples_.size(); }

    [[nodiscard]] double mean() const {
        if (samples_.empty()) return 0.0;
        double sum = 0.0;
        for (double v : samples_) sum += v;
        return sum / static_cast<double>(samples_.size());
    }

    [[nodiscard]] double percentile(double p) const {
        if (samples_.empty()) return 0.0;
        std::vector<double> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        auto idx = static_cast<std::size_t>(p * static_cast<double>(sorted.size() - 1));
        return sorted[idx];
    }

    [[nodiscard]] double total_seconds() const {
        double sum = 0.0;
        for (double v : samples_) sum += v;
        return sum / 1.0e6;
    }

private:
    time_point t0_{};
    std::vector<double> samples_;
};

}  // namespace ccsd::timing

#endif  // CCSD_TIMING_HPP
