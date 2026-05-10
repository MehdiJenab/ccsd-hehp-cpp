#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

namespace ccsd {

class Vector2D {
public:
    int rank = 0;  // MPI rank that owns/computes this tensor (set externally)

    Vector2D() = default;

    void initialization(int dim2) {
        n1_ = dim2;
        n2_ = dim2;
        n_size_ = n1_ * n2_;
        data_.assign(static_cast<std::size_t>(n_size_), 0.0);
    }

    void zeros() { std::fill(data_.begin(), data_.end(), 0.0); }

    void diagonalize(const std::vector<double>& fs_1D) {
        for (int i = 0; i < n1_; ++i) {
            for (int j = 0; j < n2_; ++j) {
                (*this)(i, j) = (i == j) ? fs_1D[static_cast<std::size_t>(i)] : 0.0;
            }
        }
    }

    [[nodiscard]] double operator()(int i, int j) const {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_);
        return data_[index(i, j)];
    }
    [[nodiscard]] double& operator()(int i, int j) {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_);
        return data_[index(i, j)];
    }

    [[nodiscard]] int n1() const noexcept { return n1_; }
    [[nodiscard]] int n2() const noexcept { return n2_; }
    [[nodiscard]] int n_size() const noexcept { return n_size_; }
    [[nodiscard]] double* raw() noexcept { return data_.data(); }
    [[nodiscard]] const double* raw() const noexcept { return data_.data(); }

    friend std::ostream& operator<<(std::ostream& os, const Vector2D& v) {
        os << "[\n";
        for (int i = 0; i < v.n1_; ++i) {
            os << "  [";
            for (int j = 0; j < v.n2_; ++j) os << v(i, j) << ",    ";
            os << "]\n";
        }
        os << "]\n\n";
        return os;
    }

private:
    [[nodiscard]] std::size_t index(int i, int j) const noexcept {
#ifdef CCSD_LAYOUT_ROW_MAJOR
        return static_cast<std::size_t>(i) * static_cast<std::size_t>(n2_)
             + static_cast<std::size_t>(j);
#else
        return static_cast<std::size_t>(j) * static_cast<std::size_t>(n1_)
             + static_cast<std::size_t>(i);
#endif
    }

    int n1_ = 0, n2_ = 0, n_size_ = 0;
    std::vector<double> data_;
};

}  // namespace ccsd

// Bring symbol into the global namespace temporarily so ccsd_code.cpp's
// existing free functions don't all need re-qualification in this ticket.
// Removed in T11.
using Vector2D = ccsd::Vector2D;
