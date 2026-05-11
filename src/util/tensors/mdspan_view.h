#pragma once

#ifdef CCSD_USE_MDSPAN

#include <util/tensors/vector_2d.h>
#include <util/tensors/vector_4d.h>

namespace ccsd {

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

}  // namespace ccsd

#endif  // CCSD_USE_MDSPAN
