#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

namespace ccsd::timing {

class PercentileAccumulator {
public:
    using clock      = std::chrono::steady_clock;
    using time_point = clock::time_point;

    struct Snapshot {
        double      mean;
        double      p50;
        double      p99;
        double      total_seconds;
        std::size_t count;
    };

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
        return percentile_of_sorted(sorted, p);
    }

    [[nodiscard]] double total_seconds() const {
        double sum = 0.0;
        for (double v : samples_) sum += v;
        return sum / 1.0e6;
    }

    [[nodiscard]] Snapshot snapshot() const {
        Snapshot s{};
        s.count = samples_.size();
        if (samples_.empty()) return s;
        std::vector<double> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        double sum = 0.0;
        for (double v : sorted) sum += v;
        s.mean          = sum / static_cast<double>(sorted.size());
        s.p50           = percentile_of_sorted(sorted, 0.50);
        s.p99           = percentile_of_sorted(sorted, 0.99);
        s.total_seconds = sum / 1.0e6;
        return s;
    }

private:
    static double percentile_of_sorted(const std::vector<double>& sorted, double p) {
        // Nearest-rank percentile: idx = ceil(p * n) - 1, clamped to [0, n-1].
        auto idx = static_cast<std::size_t>(std::ceil(p * static_cast<double>(sorted.size())));
        if (idx > 0) --idx;
        if (idx >= sorted.size()) idx = sorted.size() - 1;
        return sorted[idx];
    }

    time_point          t0_{};
    std::vector<double> samples_;
};

}  // namespace ccsd::timing
