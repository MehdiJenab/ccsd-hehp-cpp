#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace ccsd {

class Vector4D {
public:
    int rank = 0;

    Vector4D() = default;

    void initialization(int dim2) {
        n1_ = n2_ = n3_ = n4_ = dim2;
        n_size_ = n1_ * n2_ * n3_ * n4_;
        data_.assign(static_cast<std::size_t>(n_size_), 0.0);
    }

    void zeros() { std::fill(data_.begin(), data_.end(), 0.0); }

    [[nodiscard]] double operator()(int i, int j, int k, int l) const {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_ && k >= 0 && k < n3_ && l >= 0 && l < n4_);
        return data_[index(i, j, k, l)];
    }
    [[nodiscard]] double& operator()(int i, int j, int k, int l) {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_ && k >= 0 && k < n3_ && l >= 0 && l < n4_);
        return data_[index(i, j, k, l)];
    }

    [[nodiscard]] int n_size() const noexcept { return n_size_; }
    [[nodiscard]] double* raw() noexcept { return data_.data(); }
    [[nodiscard]] const double* raw() const noexcept { return data_.data(); }

private:
    [[nodiscard]] std::size_t index(int i, int j, int k, int l) const noexcept {
        [[maybe_unused]] const auto N1 = static_cast<std::size_t>(n1_);
        const auto N2 = static_cast<std::size_t>(n2_);
        const auto N3 = static_cast<std::size_t>(n3_);
        [[maybe_unused]] const auto N4 = static_cast<std::size_t>(n4_);
#ifdef CCSD_LAYOUT_ROW_MAJOR
        return ((static_cast<std::size_t>(i) * N2
               + static_cast<std::size_t>(j)) * N3
               + static_cast<std::size_t>(k)) * N4
               + static_cast<std::size_t>(l);
#else
        return static_cast<std::size_t>(l) * N1 * N2 * N3
             + static_cast<std::size_t>(k) * N1 * N2
             + static_cast<std::size_t>(j) * N1
             + static_cast<std::size_t>(i);
#endif
    }

    int n1_ = 0, n2_ = 0, n3_ = 0, n4_ = 0, n_size_ = 0;
    std::vector<double> data_;
};

}  // namespace ccsd

// Bring symbol into the global namespace temporarily so ccsd_code.cpp's
// existing free functions don't all need re-qualification in this ticket.
// Removed in T11.
using Vector4D = ccsd::Vector4D;
