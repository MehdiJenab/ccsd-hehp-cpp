# CCSD in C++ with MPI Parallelization

A C++23 implementation of Coupled Cluster with Singles and Doubles (CCSD) for
the HeH⁺ molecule, translated and modernized from the educational Python code
published by Joshua Goings.

## Repository Structure

```
ccsd-hehp-cpp/
├── cmake/            CMake modules (warnings, mdspan)
├── docs/             Documentation
│   ├── guides/       architecture.md, build.md, usage.md, workflow.md
│   ├── benchmarks/   Performance results and raw data
│   ├── design/       AI-assisted refactor specs and plans
│   └── Doxyfile
└── src/              All source code, organized by layer
    ├── util/         Generic building blocks (no CCSD knowledge)
    │   ├── tensors/  Vector2D, Vector4D, mdspan adapter
    │   └── timing/   Timer, percentile accumulator
    ├── ccsd/         CCSD-specific code (the science)
    │   ├── config/   CcsdConfig (JSON loader)
    │   ├── mpi/      MPI session, orchestrator, tensor send/recv
    │   ├── kernels/  CcsdState, CcsdKernels (pure CCSD math)
    │   └── solver/   CcsdSolver (thin coordinator)
    └── apps/         Entry points
        ├── ccsd_code.cpp, ccsd_bench.cpp
        └── scripts/  run_mpi_regression.py, run_bench.sh, plot_or_table.py

Root scaffolding: CMakeLists.txt, CMakePresets.json, Makefile, README.md, config.json
```

Each feature folder contains its public header(s), implementation, and
co-located unit tests in `tests/`. Cross-feature includes use the form
`<layer/feature/header.h>` — making the dependency direction visible at every
call site (`util/` never includes from `ccsd/`).

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
see `make help` and `docs/guides/build.md`.

## Dependencies

- C++23 compiler (GCC ≥ 13 or Clang ≥ 17)
- CMake ≥ 3.21
- OpenMPI (`mpic++`, `mpirun`)
- Python 3 (optional, for the regression script and benchmark plotting)
- nlohmann/json — fetched automatically via CMake FetchContent
- Catch2 — fetched automatically via CMake FetchContent

## Documentation

- [`docs/guides/architecture.md`](docs/guides/architecture.md) — module layout and design
- [`docs/guides/build.md`](docs/guides/build.md) — full build options and presets
- [`docs/guides/usage.md`](docs/guides/usage.md) — running the solver and bench
- [`docs/guides/workflow.md`](docs/guides/workflow.md) — development workflow
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
