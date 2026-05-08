#ifndef VectorsClass_Included
#define VectorsClass_Included

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
        return static_cast<std::size_t>(j) * static_cast<std::size_t>(n1_)
             + static_cast<std::size_t>(i);
    }

    int n1_ = 0, n2_ = 0, n_size_ = 0;
    std::vector<double> data_;
};

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
        const auto N1 = static_cast<std::size_t>(n1_);
        const auto N2 = static_cast<std::size_t>(n2_);
        const auto N3 = static_cast<std::size_t>(n3_);
        return static_cast<std::size_t>(l) * N1 * N2 * N3
             + static_cast<std::size_t>(k) * N1 * N2
             + static_cast<std::size_t>(j) * N1
             + static_cast<std::size_t>(i);
    }

    int n1_ = 0, n2_ = 0, n3_ = 0, n4_ = 0, n_size_ = 0;
    std::vector<double> data_;
};

#ifdef CCSD_USE_MDSPAN
}  // namespace ccsd
#include <experimental/mdspan>
namespace ccsd {

// View the existing column-major storage (j*n1 + i) as a 2-D mdspan.
inline auto as_mdspan(Vector2D& v) {
    using ext = std::experimental::extents<int,
        std::experimental::dynamic_extent,
        std::experimental::dynamic_extent>;
    return std::experimental::mdspan<double, ext, std::experimental::layout_left>(
        v.raw(), v.n1(), v.n2());
}

// 4-D, column-major: index(i,j,k,l) = ((l*N3 + k)*N2 + j)*N1 + i.
inline auto as_mdspan(Vector4D& v) {
    using ext = std::experimental::extents<int,
        std::experimental::dynamic_extent,
        std::experimental::dynamic_extent,
        std::experimental::dynamic_extent,
        std::experimental::dynamic_extent>;
    int n = v.n_size();  int dim = 0;
    for (int d = 1; (d * d * d * d) <= n; ++d) dim = d;
    return std::experimental::mdspan<double, ext, std::experimental::layout_left>(
        v.raw(), dim, dim, dim, dim);
}

#endif  // CCSD_USE_MDSPAN

}  // namespace ccsd

// Bring symbols into the global namespace temporarily so ccsd_code.cpp's
// existing free functions don't all need re-qualification in this ticket.
// Removed in T11.
using Vector2D = ccsd::Vector2D;
using Vector4D = ccsd::Vector4D;

#endif  // VectorsClass_Included
