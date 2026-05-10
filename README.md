# CCSD in C++ with MPI Parallelization

A C++23 implementation of Coupled Cluster with Singles and Doubles (CCSD) for
the HeH⁺ molecule, translated and modernized from the educational Python code
published by Joshua Goings.

## Repository Structure

```
ccsd-hehp-cpp/
├── apps/             Executables (ccsd_code, ccsd_bench)
├── cmake/            CMake modules (warnings, mdspan)
├── docs/             Documentation and design specs
│   ├── architecture.md, build.md, usage.md, workflow.md
│   ├── benchmarks/   Performance results and raw data
│   ├── superpowers/  AI-assisted refactor specs and plans
│   └── Doxyfile
├── scripts/          Utility scripts (bench runner, plot, config migration)
├── src/              Source code, organized by feature
│   ├── tensors/      Vector2D, Vector4D, mdspan adapter
│   ├── mpi/          MPI session, orchestrator, tensor send/recv
│   ├── config/       CcsdConfig (JSON loader)
│   ├── kernels/      CcsdState, CcsdKernels (pure CCSD math)
│   ├── solver/       CcsdSolver (thin coordinator)
│   └── timing/       Timer, percentile accumulator
├── tests/            MPI regression tests (run_mpi_regression.py)
├── CMakeLists.txt    Top-level build
├── CMakePresets.json Build presets (debug, release, asan, ...)
├── Makefile          Convenience wrappers around cmake/ctest
└── config.json       HeH⁺/STO-3G input data (loaded by ccsd_code at runtime)
```

Each feature folder under `src/` contains its public header(s), implementation,
and co-located unit tests in `tests/`. Public APIs live at the feature root;
internal helpers live in `detail/` subfolders. Cross-feature includes use the
form `#include <feature/header.h>`, making dependency direction explicit.

## Quick Start

```bash
# Configure + build (debug preset, default)
make build

# Run unit tests (Catch2)
make test

# Run bit-exact MPI regression (np = 2, 4, 8)
make regression

# Run the full quality suite (format + tidy + tests + regression)
make check
```

For other build modes (asan, tsan, coverage, release-fast, PGO, OpenMP, mdspan)
see `make help` and `docs/build.md`.

## Dependencies

- C++23 compiler (GCC ≥ 13 or Clang ≥ 17)
- CMake ≥ 3.21
- OpenMPI (`mpic++`, `mpirun`)
- Python 3 (optional, for the regression script and benchmark plotting)
- nlohmann/json — fetched automatically via CMake FetchContent
- Catch2 — fetched automatically via CMake FetchContent

## Documentation

- [`docs/architecture.md`](docs/architecture.md) — module layout and design
- [`docs/build.md`](docs/build.md) — full build options and presets
- [`docs/usage.md`](docs/usage.md) — running the solver and bench
- [`docs/workflow.md`](docs/workflow.md) — development workflow
- [`docs/benchmarks/`](docs/benchmarks/) — performance studies

## References

- T. Daniel Crawford and Henry F. Schaefer III, "An introduction to coupled
  cluster theory for computational chemists," *Reviews in Computational
  Chemistry*, vol. 14, pp. 33–136, 2007.
- J. F. Stanton, J. Gauss, J. D. Watts, R. J. Bartlett, "A direct product
  decomposition approach for symmetry exploitation in many-body methods,"
  *J. Chem. Phys.* 94, 4334 (1991) — equations (1)–(10) implemented in
  `src/kernels/ccsd_kernels.cpp`.
- J. Goings, "Coupled Cluster with Singles and Doubles (CCSD) in Python,"
  https://joshuagoings.com/2013/07/17/coupled-cluster-with-singles-and-doubles-ccsd-in-python/

## License

MIT License.
