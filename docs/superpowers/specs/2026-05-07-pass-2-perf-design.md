# Modernization Pass 2 — Performance / HPC Throughput Design Spec

**Author:** Mehdi Jenab
**Date:** 2026-05-07
**Repo:** `ccsd-hehp-cpp`
**Prereq:** `pass-1` merged into `main` (12-ticket modernization complete; bit-exact gate
holds for HeH⁺/STO-3G).
**Scope:** Pass 2 = performance / HPC research on the same problem (HeH⁺ in STO-3G),
treated as a CCSD throughput kernel.

---

## 1. Goal and invariant

Make `ccsd-hehp-cpp` measurably faster as a CCSD throughput kernel by exploring
three optimization knobs — compiler flags + PGO, tensor layout / `mdspan`, and
OpenMP-within-ranks + MPI tuning — on this node and a small cluster. Deliverable:
optimized code on `pass-2` branch and `docs/benchmarks/pass-2-results.md` with a
speedup table.

**Two-tier regression gate (the contract):**

1. **Strict tier** — canonical build (MPI-only, no OpenMP, no `-ffast-math`).
   Every commit must keep
   ```
     E(corr,CCSD) = -0.008225832259
     E(CCSD) = -2.862598243
   ```
   bit-exact for `np ∈ {2, 4, 8}` (same gate as pass 1).
2. **Tolerance tier** — perf builds (OpenMP, mdspan, `-O3 -march=native`):
   `|E_corr − (−0.008225832259)| < 1e-9` and `|E_total − (−2.862598243)| < 1e-9`.

CI runs both tiers; both must be green. A perf change that breaks even the
tolerance tier is wrong, not the gate.

## 2. Non-goals (pass 2)

- No BLAS / cuBLAS / Eigen-backed contractions. Tensor sizes (4⁴ = 256 doubles)
  are too small for BLAS to help.
- No GPU offload (CUDA / SYCL / OpenMP-target). Pass 3 territory.
- No support for molecules other than HeH⁺ in STO-3G. Workload is the
  throughput batch, not larger systems.
- No new file formats; `config.json` schema unchanged.
- No relaxing of `-Werror` or the warning suppressions added in T12.
- No `-ffast-math` / `-Ofast`. They permit unaudited reordering.
- No hand-written SIMD intrinsics. We rely on `-O3 -march=native` autovectorization.

## 3. Method

- **Wave-based ticket execution.** Six waves, ~14 tickets total, GitHub Issues
  on the repo. Each ticket = one PR.
- **Measure-first.** Wave 1 establishes infrastructure and a baseline number.
  Every subsequent ticket records its impact in `pass-2-results.md`.
- **Two-tier gate runs before every commit.** Strict tier always; tolerance
  tier whenever the ticket touches a perf preset.
- **`make bench` is reproducible.** All numbers in the report come from the
  canonical harness on a clean preset build. The harness produces JSON; a
  Python collator turns JSON into the markdown table.
- Branch: `pass-2` off `main` after `pass-1` is merged.

## 4. Toolchain

| Tool | Choice | Notes |
|---|---|---|
| Compiler | GCC 13.1 via `mpicxx` | Same as pass 1 |
| OpenMP | libgomp (GCC 13.1) | OpenMP 4.5 |
| `mdspan` | Kokkos single-header reference impl | libstdc++ 13.1 has no `<mdspan>`; FetchContent a pinned commit |
| Profiling | Linux `perf` | Fallback: `gprof`; final fallback: `#ifdef CCSD_TIMING` instrumentation |
| Bench harness | shell + Python | `benchmarks/run_bench.sh` writes JSON; `benchmarks/plot_or_table.py` collates |
| CI | GitHub Actions | Existing matrix extended with `release-fast` and tolerance-tier jobs |

## 5. Architecture & components

```
ccsd-hehp-cpp/
├── apps/
│   ├── ccsd_code.cpp                 # unchanged (single-shot)
│   └── ccsd_bench.cpp                # NEW — throughput-batch driver
├── include/ccsd/
│   ├── ccsd_solver.hpp               # touched: hot loops parameterized on layout
│   ├── tensors.hpp                   # touched: optional mdspan-backed Vector2D/4D
│   ├── timing.hpp                    # NEW — high-res Timer + accumulator
│   └── (existing headers unchanged)
├── benchmarks/
│   ├── run_bench.sh                  # NEW — wraps mpirun + ccsd_bench, dumps JSON
│   └── plot_or_table.py              # NEW — JSON → markdown table
├── docs/benchmarks/
│   ├── pass-2-results.md             # NEW — running report
│   ├── methodology.md                # NEW — reproducibility steps
│   └── profile-baseline.md           # NEW — perf report for the baseline
├── CMakePresets.json                 # extended: release-fast{,-pgo}, *-mdspan, *-omp
├── tests/run_mpi_regression.py       # touched: --tolerance flag
└── (everything else from pass 1 unchanged)
```

**New CMake presets layered on the pass-1 set:**

| Preset | Purpose | Flags |
|---|---|---|
| `release` (existing) | Pass-1 default; strict tier | `-O2 -DNDEBUG` |
| `release-fast` | Wave 2 baseline | `-O3 -march=native -flto -DNDEBUG` |
| `release-fast-pgo` | Wave 2 PGO (two-stage) | above + `-fprofile-use=…` |
| `release-mdspan` | Wave 3 mdspan-backed tensors | `release-fast` + `-DCCSD_USE_MDSPAN=ON` |
| `release-omp` | Wave 4 OpenMP-in-rank | `release-fast` + `-fopenmp` + `-DCCSD_USE_OMP=ON` |
| `release-omp-mdspan` | Wave 4 union | both switches |

Strict tier runs against `debug` and `release`. Tolerance tier runs against
`release-fast` and onward.

**`ccsd_bench` CLI:**

```
ccsd_bench --batch N --warmup K [--threads-per-rank T] [--report path.json]
```

JSON schema:
```json
{
  "preset": "release-fast", "np": 4, "threads_per_rank": 1,
  "batch": 1000, "warmup": 50,
  "wall_seconds": 4.123,
  "per_iter_us": {"mean": 4123, "p50": 4100, "p99": 4380},
  "energies": {"E_corr": -0.008225832259, "E_total": -2.862598243},
  "git_sha": "...", "host": "...", "build_type": "Release", "march": "native"
}
```

`ccsd_bench` re-initializes `CcsdSolver` between iterations so each iteration
is independent. With `--batch 1 --warmup 0`, output must be identical to
`ccsd_code` running once.

**Solver-side switches (compile-time, not runtime):**

- `tensors.hpp`: when `CCSD_USE_MDSPAN` is defined, `Vector2D/4D` are typedef'd
  to `std::mdspan`-wrapper types over the same `std::vector<double>` storage.
  Memory layout (column-major, `j*n1 + i`) is preserved by default. Alternative
  layouts go behind `CCSD_LAYOUT_ROW_MAJOR`.
- `ccsd_solver.hpp`: hot inner loops gain `#pragma omp parallel for` (with
  `reduction(+:…)` where needed) under `#ifdef CCSD_USE_OMP`. Loops are picked
  from the wave-1 `perf` profile (P04).

## 6. Ticket backlog

### Wave 1 — Infrastructure & baseline (5 tickets)

**P01. `ccsd_bench` throughput driver.**
- New `apps/ccsd_bench.cpp` wrapping `CcsdSolver` in a `--batch N --warmup K`
  loop. New `include/ccsd/timing.hpp` with `chrono`-based percentile
  accumulator. CMake target `ccsd_bench`. Energies of last iteration printed
  identically to `ccsd_code` for regression compatibility.
- Acceptance: `ccsd_bench --batch 100 --warmup 10` prints the canonical energy
  lines and a per-iter mean.

**P02. Two-tier regression gate.**
- `tests/run_mpi_regression.py` gains `--tolerance T` flag. Default `0` =
  bit-exact (unchanged).
- Add CI job `tolerance` that runs `--tolerance 1e-9` against the
  `release-fast` build (created in P05).
- Acceptance: existing strict gate green; deliberate sub-`1e-9` perturbation
  in the print rejected by strict but accepted by tolerance.

**P03. Benchmark harness.**
- `benchmarks/run_bench.sh` runs the parameter matrix (preset × np × threads)
  and dumps one JSON per row.
- `benchmarks/plot_or_table.py` collates JSONs into a markdown table.
- `make bench` and `make bench-quick` targets.
- Acceptance: `make bench` produces `docs/benchmarks/raw/*.json` and refreshes
  `docs/benchmarks/pass-2-results.md`.

**P04. Profile the baseline (perf).**
- Run `perf record -F 999 -g -- mpirun -np 4 build/release/ccsd_bench
  --batch 1000 --warmup 50`. Capture `perf report --stdio` output into
  `docs/benchmarks/profile-baseline.md`.
- Acceptance: report names the top-10 hottest functions.

**P05. `release-fast` preset.**
- New CMake preset with `-O3 -march=native -flto -DNDEBUG`.
- CI runs `release-fast` through tolerance-tier regression.
- First measured speedup recorded in `pass-2-results.md`.
- Acceptance: `cmake --preset release-fast && cmake --build --preset
  release-fast && ctest --preset release-fast` green; tolerance tier green.

### Wave 2 — Compiler PGO (1 ticket)

**P06. `release-fast-pgo` two-stage preset.**
- Stage A: `-fprofile-generate=…`, run `ccsd_bench --batch 200`.
- Stage B: `-fprofile-use=…`.
- `make bench-pgo` wraps the two-stage workflow.
- Speedup recorded.
- Acceptance: PGO rebuild succeeds; tolerance tier green; speedup row appended
  (positive or negative is fine — it's a recorded finding).

### Wave 3 — Tensor layout / mdspan (3 tickets)

**P07. Pull in `mdspan` via FetchContent.**
- Kokkos single-header reference impl (`mdspan.hpp`), pinned commit, into
  `_deps/`. `cmake/ccsd_mdspan.cmake` exposes `target_link_libraries(...
  ccsd::mdspan)`.
- Acceptance: a smoke test in `tests/test_tensors.cpp` constructs an
  `std::mdspan` over a `std::vector<double>` and reads it back.

**P08. `Vector2D/4D` opt-in mdspan backing (column-major preserved).**
- When `CCSD_USE_MDSPAN=ON`, the tensor types become `std::mdspan` wrappers
  over the same storage with the same column-major layout (`j*n1 + i`),
  preserving memory layout exactly.
- New preset `release-mdspan`.
- Acceptance: `ccsd_code` built with `release-mdspan` passes **strict-tier**
  regression (same FP order, same memory layout).

**P09. Layout study: column- vs row-major.**
- Add `CCSD_LAYOUT_ROW_MAJOR` switch alongside `CCSD_USE_MDSPAN`.
- Two new bench rows: `release-mdspan-colmajor`, `release-mdspan-rowmajor`.
- Row-major changes traversal order → tolerance tier only.
- Acceptance: both layouts pass tolerance-tier regression; report has both.

### Wave 4 — OpenMP within ranks (2 tickets)

**P10. OpenMP pragmas on hot loops.**
- Pick top-3 hottest loops from P04. Add `#pragma omp parallel for` (with
  `reduction(+:…)` where needed) under `#ifdef CCSD_USE_OMP`.
- New preset `release-omp` (`-fopenmp -DCCSD_USE_OMP=ON`).
- Acceptance: with `OMP_NUM_THREADS=1`, results match strict tier (sanity
  check); with `OMP_NUM_THREADS=4`, tolerance tier green.

**P11. OpenMP × MPI matrix sweep.**
- Bench rows for (np, threads) ∈ {(1,1), (1,4), (2,2), (4,1), (4,4), (8,1)}.
- Identify (np, threads) sweet spot for this node.
- Acceptance: report has the matrix; commentary names the best row.

### Wave 5 — MPI tuning (2 tickets)

**P12. NUMA pinning + binding.**
- `benchmarks/run_bench.sh` adds `--bind-to core --map-by socket:PE=$T` for
  the OpenMP variant. Compare unpinned vs pinned.
- Acceptance: report has both unpinned and pinned numbers.

**P13. Collective-ops + message coalescing audit.**
- Replace point-to-point `Send/Recv` patterns inside `update_intermediates`
  (where applicable) with `MPI_Allreduce` / `MPI_Bcast`.
- Reduction order may change → tolerance tier only.
- Acceptance: tolerance-tier green; bench shows MPI-time fraction (from
  `MPI_T_pvar` if available, otherwise wall-time delta).

### Wave 6 — Cluster validation + report (1 ticket)

**P14. Multi-node scaling study + final report.**
- If a cluster is available: run `release-fast-pgo + omp + pinning` on N nodes
  for N ∈ {1, 2, 4} (best-effort).
- If no cluster: explicitly note that and run only on this node.
- Final `pass-2-results.md` consolidated: baseline → release-fast → PGO →
  mdspan → row-major → OpenMP → pinning → collective-ops → multi-node.
- Acceptance: report committed; both gates green for all entries; PR opened
  against `main`.

## 7. Methodology — how a number gets into the report

1. **Warm phase**: discard first K iterations (default K = 50) to populate
   caches and stabilize CPU frequency.
2. **Measure phase**: run N iterations (default N = 1000), record per-iteration
   wall time via `chrono::steady_clock`.
3. **Statistics**: mean, median (p50), p99. Report **median**, not mean —
   robust to GC-style spikes.
4. **Repetitions**: each (preset, np, threads) row is run 3 times back-to-back;
   record **best of 3 medians**. Mitigates one-off OS noise.
5. **Frequency stability**: warmup includes a 200ms `__rdtsc()`-busy spin to
   escape the CPU governor's idle state. Documented in `methodology.md`.
6. **Single source of baseline**: P05 records the `release-fast` median once
   and pins it. Every subsequent ticket reports speedup as
   `baseline_median / new_median`.

Speedups < 1.0× still get recorded — negative results are findings.

## 8. Pre-commit discipline (operating rule)

```
# Strict tier — always
cmake --build --preset debug
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code

# Tolerance tier — for any ticket that touches a perf preset
cmake --build --preset <perf-preset>
python3 tests/run_mpi_regression.py --executable build/<perf-preset>/ccsd_code \
    --tolerance 1e-9

# Both must end with: "All MPI regression tests passed."
```

If a wave introduces a new preset (P05, P08, P10, P12), the ticket also runs
**strict tier** as a sanity check (using whatever build is OpenMP-disabled /
single-threaded equivalent). The strict tier never gets relaxed.

## 9. CI matrix (GitHub Actions, Ubuntu 22.04, GCC 13)

Existing pass-1 jobs preserved (`debug`, `asan`, `coverage`, `tidy`). Added:

1. **release-fast** — configure → build → ctest → `python3
   tests/run_mpi_regression.py --executable build/release-fast/ccsd_code
   --tolerance 1e-9`. Active from P05.
2. **release-omp** — same, with `OMP_NUM_THREADS=4`. Active from P10.
3. **bench-smoke** — runs `ccsd_bench --batch 50 --warmup 10` to confirm the
   harness compiles and executes; does not gate on numeric speed.

## 10. Numerical-determinism rules (pass 2)

- Strict tier never reorders FP operations.
- Tolerance tier may reorder via OpenMP `reduction`, mdspan iteration order,
  MPI collectives. Each must stay within `1e-9` absolute.
- `-ffast-math` and `-Ofast` remain banned.
- No hand-written SIMD intrinsics.
- Reduction order in MPI collectives is platform-defined. We accept this
  within the tolerance tier; OpenMPI version recorded in each JSON row.

## 11. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Throughput batch hides bugs that one-shot exposes | `ccsd_bench` re-initializes `CcsdSolver` between iters; `--batch 1 --warmup 0` must match `ccsd_code` |
| `mdspan` reference impl behaves differently than future libstdc++ `<mdspan>` | Pin a Kokkos commit; T07 unit test catches drift |
| OpenMP reduction order drifts past `1e-9` | Narrow parallelization to outer loops; or use `omp ordered` deterministic reduction at perf cost |
| Cluster unavailability blocks P14 | P14 may be node-only with a written caveat; not a blocker |
| `perf` not installed | P04 degrades: `perf stat` → `gprof` → manual `#ifdef CCSD_TIMING` instrumentation |
| `-O3` surfaces UB the previous build hid | Strict tier on `release` is the canary; if a `release-fast` strict-equivalent fails, real bug to fix first |

## 12. Out of scope (pass-3 candidates)

- BLAS / `cblas_dgemm`-backed contractions (only justifiable at larger N).
- GPU offload (CUDA / SYCL / OpenMP-target).
- Generalize beyond HeH⁺ in STO-3G (synthetic-input mode for arbitrary
  `dim`/`Nelec`).
- Replace hand-roll with `Eigen::Tensor` (large rewrite, not justified at this
  N).
- Auto-tuning of `(np, threads)` per host. We record the matrix; humans pick.
