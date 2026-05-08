#include <catch2/catch_test_macros.hpp>

#include <ccsd/tensors.hpp>
#include <experimental/mdspan>

TEST_CASE("Vector2D index encoding matches legacy layout", "[tensor][2d]") {
    ccsd::Vector2D v;
    int dim = 4;
    v.initialization(dim);

    for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j)
            v(i, j) = static_cast<double>(j * dim + i);  // matches old j*n1 + i

    REQUIRE(v.raw()[0] == 0.0);                         // (0,0)
    REQUIRE(v.raw()[1] == 1.0);                         // (1,0)  -> j=0, i=1
    REQUIRE(v.raw()[dim] == static_cast<double>(dim));  // (0,1)  -> j=1, i=0

    for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j)
            REQUIRE(v(i, j) == static_cast<double>(j * dim + i));
}

TEST_CASE("Vector4D index encoding matches legacy layout", "[tensor][4d]") {
    ccsd::Vector4D t;
    int dim = 3;
    t.initialization(dim);

    for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j)
            for (int k = 0; k < dim; ++k)
                for (int l = 0; l < dim; ++l)
                    t(i, j, k, l) = static_cast<double>(l * dim * dim * dim
                                                        + k * dim * dim + j * dim + i);

    REQUIRE(t.raw()[0]                 == 0.0);                                   // (0,0,0,0)
    REQUIRE(t.raw()[1]                 == 1.0);                                   // (1,0,0,0)
    REQUIRE(t.raw()[dim]               == static_cast<double>(dim));              // (0,1,0,0)
    REQUIRE(t.raw()[dim * dim]         == static_cast<double>(dim * dim));        // (0,0,1,0)
    REQUIRE(t.raw()[dim * dim * dim]   == static_cast<double>(dim * dim * dim));  // (0,0,0,1)
}

TEST_CASE("Vector2D::zeros resets storage", "[tensor][zeros]") {
    ccsd::Vector2D v;
    v.initialization(3);
    v(1, 2) = 7.0;
    v.zeros();
    REQUIRE(v(1, 2) == 0.0);
    REQUIRE(v(0, 0) == 0.0);
}

TEST_CASE("Vector2D::diagonalize places vector on the diagonal", "[tensor][diag]") {
    ccsd::Vector2D v;
    v.initialization(3);
    std::vector<double> diag{1.0, 2.0, 3.0};
    v.diagonalize(diag);
    REQUIRE(v(0, 0) == 1.0);
    REQUIRE(v(1, 1) == 2.0);
    REQUIRE(v(2, 2) == 3.0);
    REQUIRE(v(0, 1) == 0.0);
    REQUIRE(v(2, 0) == 0.0);
}

TEST_CASE("mdspan reference impl is available", "[mdspan][smoke]") {
    std::vector<double> storage(12, 0.0);
    using extents_t = std::experimental::extents<int, 3, 4>;
    std::experimental::mdspan<double, extents_t> view(storage.data());
    view(1, 2) = 7.0;
    REQUIRE(storage[1 * 4 + 2] == 7.0);  // default layout_right (row-major)
    REQUIRE(view.extent(0) == 3);
    REQUIRE(view.extent(1) == 4);
}
