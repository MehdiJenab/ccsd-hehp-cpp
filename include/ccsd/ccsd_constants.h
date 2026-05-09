#pragma once

namespace ccsd::constants {
    // MPI message tags — chosen to avoid collision between Vector2D and Vector4D sends.
    constexpr int mpi_tag_2d = 233;
    constexpr int mpi_tag_4d = 610;
    // CCSD amplitude convergence criterion (energy difference threshold).
    constexpr double convergence_threshold = 10e-9;
}
