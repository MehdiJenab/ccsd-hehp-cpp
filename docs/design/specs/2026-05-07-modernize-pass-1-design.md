# Modernization Pass 1 — Design Spec

**Author:** Mehdi Jenab
**Date:** 2026-05-07
**Repo:** `ccsd-hehp-cpp`
**Scope:** Pass 1 of a two-pass modernization. Pass 1 = code quality only.
Pass 2 = performance / HPC research (separate later spec).

---

## 1. Goal and invariant

Modernize `ccsd-hehp-cpp` for code quality only — no algorithm changes, no
scope expansion, no perf tuning — using **C++23** built with **GCC 13.1** via
`mpic++ 13.1` on this machine.

The hard contract for every commit on `main`, every PR, and every ticket:

```
mpirun --oversubscribe -np {2,4,8} ./ccsd_code
```

must print **exactly** the two lines below — string-equality, not numerical
tolerance:

```
E(corr,CCSD) = -0.008225832259
E(CCSD) = -2.862598243
```

If a refactor changes floating-point ordering enough to flip the last digit,
that is a **signal** that the refactor changed semantics. The ticket gets
reworked, not the gate loosened. Tolerance relaxation is a pass-2 concern.

## 2. Non-goals (pass 1)

- No support for molecules / basis sets other than HeH⁺ in STO-3G.
- No Eigen, BLAS, `<mdspan>`, GPU, OpenMP, or any new parallelization layer.
- No change to the MPI decomposition strategy.
- No Python bindings, no new file formats.
- No Doxygen overhaul (light edits only to keep it building after the layout
  migration in T11).
- No `<print>`, `<mdspan>`, `<flat_map>`, `<generator>`, `<stacktrace>` —
  libstdc++ 13.1 lacks them. Permitted C++23: `<expected>`, `<ranges>`,
  `<span>`, `<format>`, `<numbers>`, deducing-this, `if consteval`, multidim
  subscript.

## 3. Method

- **Ticket-driven.** GitHub Issues on the repo; each ticket is a single PR.
  Working branch: `pass-1` off the current `main` HEAD.
- **Bit-exact regression** is the safety net. Tightened in T01 *before*
  any code is touched.
- **No big-bang rewrite.** New scaffolding lives alongside legacy build until
  T11; legacy build is deleted at T11.
- **Warnings staged.** Full `-W…` flag set added in T02 as warnings only;
  `-Werror` flipped on as the closing ticket T12.
- **Substrate is provisional.** Tensor / parameter / MPI APIs designed to be
  *swapped*, not preserved. Pass 2 may replace them.

## 4. Toolchain

| Tool | Choice | Notes |
|---|---|---|
| Compiler | GCC 13.1 via `mpic++` | Verified on this node |
| C++ standard | C++23 | `set(CMAKE_CXX_STANDARD 23)`, `CMAKE_CXX_EXTENSIONS OFF` |
| Build generator | Ninja | Via `CMakePresets.json` |
| Test framework | Catch2 v3 | FetchContent |
| External deps | FetchContent only | nlohmann/json + Catch2 are the only adds |
| Format | clang-format | `.clang-format`, LLVM base, ColumnLimit 100, c++23 |
| Lint | clang-tidy | `.clang-tidy`, blocking from T12 |
| Sanitizers | ASan + UBSan + TSan | Per-preset opt-in |
| Coverage | gcovr | `--fail-under-line 100`, gate from T11 |
| Pre-commit | pre-commit | clang-format on staged C++ from T04; clang-tidy from T12 |
| CI | GitHub Actions | Ubuntu 22.04, GCC 13 |

## 5. Target repository layout (end of pass 1)

```
ccsd-hehp-cpp/
├── .claude/
│   ├── CLAUDE.md
│   ├── settings.local.json           # gitignored
│   └── agents/ccsd-reviewer.md
├── .github/workflows/ci.yml
├── apps/ccsd_code.cpp                 # thin main()
├── cmake/ProjectWarnings.cmake
├── include/ccsd/
│   ├── tensors.hpp                    # was VectorsClass.h
│   ├── parameters.hpp                 # was ParameterClass.h
│   ├── mpi_session.hpp                # was MpiClass.h, RAII
│   ├── intermediates.hpp
│   ├── amplitudes.hpp
│   ├── energy.hpp
│   └── driver.hpp                     # CcsdSolver replaces globals
├── src/
│   ├── CMakeLists.txt
│   ├── tensors.cpp
│   ├── parameters.cpp
│   ├── mpi_session.cpp
│   ├── intermediates.cpp
│   ├── amplitudes.cpp
│   ├── energy.cpp
│   └── driver.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_tensors.cpp
│   ├── test_parameters.cpp
│   ├── test_mpi_session.cpp
│   ├── test_intermediates.cpp
│   └── run_mpi_regression.py          # tightened to bit-exact in T01
├── docs/
│   ├── architecture.md
│   ├── usage.md
│   ├── workflow.md
│   └── superpowers/specs/2026-05-07-modernize-pass-1-design.md
├── config.json
├── CMakeLists.txt                     # rewritten, presets-driven
├── CMakePresets.json
├── Makefile                           # thin wrapper over presets
├── .clang-format
├── .clang-tidy
├── .pre-commit-config.yaml
├── .gitignore
├── Doxyfile                           # paths updated
└── README.md
```

**Deleted by end of pass 1:** `ParameterClass_NoJson.h`, the macro defines
`iLoop`/`jLoop`/`mLoop`/`nLoop`/`eLoop`/`fLoop`/`aLoop`/`bLoop` in
`ccsd_code.cpp`. (No legacy `Makefile` exists in this checkout to delete.)

**Stable across pass 1:** the `config.json` schema (keys may be re-keyed by
name but no field is added/removed), the `tests/run_mpi_regression.py`
invocation pattern, the printed energy lines.

## 6. Ticket backlog

Each ticket = one GitHub issue = one PR. After every ticket the bit-exact
regression must be green for `np ∈ {2, 4, 8}`.

### Group A — Safety net & tooling

**T01. Tighten the regression to bit-exact + verify it can fail.**
- Replace tolerance check in `tests/run_mpi_regression.py` with exact string
  match against the two energy lines.
- Demonstrate it fails on a deliberate format perturbation, then revert.
- Acceptance: regression exits 0 normally; deliberate perturbation makes it
  exit non-zero; revert restores green.

**T02. Bootstrap modern build alongside legacy.**
- Add `CMakePresets.json` (debug, release, asan, tsan, coverage).
- Add `cmake/ProjectWarnings.cmake` with the full warning set, **no
  `-Werror`**.
- Add `.clang-format`, `.clang-tidy` (latter not yet enforced).
- Add `Makefile` wrapper: `build`, `test`, `asan`, `coverage`, `format`,
  `tidy`, `check`.
- The existing `CMakeLists.txt` keeps working unchanged. (There is **no
  legacy Makefile** in this checkout despite the README; ignore the README's
  Makefile references.)
- Acceptance: `cmake --preset debug && cmake --build --preset debug && ctest
  --preset debug` green; bit-exact regression green; the legacy
  `cmake -S . -B build && cmake --build build` flow still works.

**T03. GitHub Actions CI.**
- `.github/workflows/ci.yml`: jobs for debug+ctest+regression, asan+regression,
  coverage+regression. Ubuntu 22.04, GCC 13.
- Acceptance: CI green on a no-op PR.

**T04. Per-project `.claude/` and pre-commit hooks.**
- Add `.claude/CLAUDE.md`, gitignored `.claude/settings.local.json`, one
  reviewer agent under `.claude/agents/`.
- `.pre-commit-config.yaml`: clang-format on staged C++. **No clang-tidy in
  pre-commit yet** (too slow until layout settles in T11).
- Acceptance: `pre-commit run --all-files` passes; mechanical reformat lands
  in this ticket.

### Group B — Code-quality cleanup (current layout)

**T05. Drop `using namespace std` from headers; remove `ParameterClass_NoJson.h`.**
- All `*.h` files use `std::` qualification. `using namespace std;` may
  remain inside `ccsd_code.cpp`.
- Delete `ParameterClass_NoJson.h` and any references; document JSON as
  required.
- Acceptance: build clean; bit-exact green.

**T06. RAII MPI session.**
- Replace `MpiClass` with `ccsd::MpiSession`: ctor calls `MPI_Init`, dtor
  calls `MPI_Finalize`. `rank` and `size` are const members. No globals.
- Acceptance: `git grep MPI_Init && git grep MPI_Finalize` only match
  `mpi_session.{hpp,cpp}`; bit-exact green.

**T07. Fix tensor leaf API.**
- In `Vector2D` / `Vector4D`: fix bounds (`>` → `>=`); replace raw-pointer
  `set()` with `operator()` returning a non-const reference (deducing-this
  for read/write); `initialization(int&)` → `initialization(int)`; add
  approx-equal helpers; remove `using namespace std`; correct typo
  `Vector2DClas_Included`.
- Decouple MPI: move `send`/`recv`/`mpi_bcast` off the tensor classes into
  free functions `ccsd::mpi::send(t, dst)` etc.
- Delete dead error branches that print `"error, out of range"` and return
  `NULL`; replace with `assert` preconditions.
- Acceptance: bit-exact green; new Catch2 tests for tensor index math and
  bounds passing.

**T08. Replace macro loop bundles with named iteration helpers.**
- Drop the macro defines. Introduce small lambdas or index-range helpers
  capturing `Nelec` and `dim2`. Algebraic expressions stay 1-1.
- Acceptance: `git grep -nE '#define [ijmnefab]Loop' ccsd_code.cpp` empty;
  bit-exact green.

**T09. Eliminate file-scope globals — introduce `CcsdSolver`.**
- Move `Fae`, `Fmi`, …, `td`, `spinints`, `fs`, `p`, `mpi`, `dim2`,
  `rank_master`, `rank_start` into a `CcsdSolver` class (or aggregate state
  + free functions, decided in implementation).
- `main()` becomes: parse params → construct solver → run iterations →
  print energies.
- Acceptance: only file-scope object remaining is `int main(int, char**)`;
  bit-exact green.

**T10. ParameterClass: name-keyed JSON, switch to nlohmann/json.**
- Replace jansson with nlohmann/json (FetchContent, single header).
- Read by name (`config["dim"]`) instead of fixed array indices.
- `ttmo` is no longer `std::map<double,double>`; expose a typed accessor
  that computes the compound index inside.
- Acceptance: `apt install libjansson-dev` no longer required; `config.json`
  schema documented in `docs/usage.md`; bit-exact green.

### Group C — Layout migration & enforcement

**T11. Migrate to `include/ccsd/` + `src/` + `apps/` layout.**
- Split `ccsd_code.cpp` into `intermediates.cpp`, `amplitudes.cpp`,
  `energy.cpp`, `driver.cpp` plus matching headers.
- Move tensors / parameters / mpi_session into the new tree.
- Thin `apps/ccsd_code.cpp` calls into the library.
- Replace the top-level `CMakeLists.txt` with the new presets-driven
  version (don't keep two CMake files in parallel).
- Acceptance: bit-exact green for `np ∈ {2,4,8}`; gcovr reports ≥100% line
  coverage on `src/` and `include/ccsd/` from unit tests + regression run
  combined.

**T12. Flip `-Werror` on; turn on clang-tidy in CI and pre-commit.**
- `cmake/ProjectWarnings.cmake` adds `-Werror`. CI's clang-tidy job becomes
  blocking. Pre-commit runs clang-tidy on staged files.
- Any leftover warnings get fixed in this ticket.
- Acceptance: `make check` green from a clean clone; bit-exact green;
  pre-commit blocks deliberately ill-formatted or lint-failing changes.

## 7. Build modes

| Preset | Flags | Purpose |
|---|---|---|
| `debug` | `-O0 -g` + warning set | Inner loop |
| `release` | `-O2 -DNDEBUG` | Default user build |
| `asan` | `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` | UB / memory |
| `tsan` | `-O1 -g -fsanitize=thread` | Race detection (may be exempt from CI if MPI fights it) |
| `coverage` | `-O0 -g --coverage` | gcovr report |

**Initial warning set (T02), no `-Werror` until T12:**
`-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast`
`-Wcast-align -Woverloaded-virtual -Wconversion -Wsign-conversion`
`-Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough`.

## 8. CI matrix (GitHub Actions, Ubuntu 22.04, GCC 13)

1. **debug** — configure → build → ctest → `python3 tests/run_mpi_regression.py`.
2. **asan+ubsan** — configure → build → ctest → regression run under sanitizers
   (smaller `np` if needed to fit runner).
3. **coverage** — configure → build → ctest + regression →
   `gcovr --fail-under-line 100` from T11 onward (warn-only before).
4. **clang-tidy** (from T12) — blocking lint over `src/` and `include/ccsd/`.

## 9. Test layering

- **Unit tests (Catch2 v3):** `test_tensors.cpp`, `test_parameters.cpp`,
  `test_mpi_session.cpp` (single-rank only), `test_intermediates.cpp` (small
  hand-checked inputs).
- **End-to-end regression:** `tests/run_mpi_regression.py` — bit-exact match
  on the two energy lines for `np ∈ {2, 4, 8}` with `--oversubscribe`. The
  contract.
- **Coverage:** unit tests + regression run together feed one merged gcov
  report. Gate at 100% line coverage from T11.

## 10. Numerical-determinism rules during pass 1

- Iteration order over indices must not change.
- No reassociating sums (`a+b+c` vs `(a+b)+c`).
- No `-ffast-math`, no `-Ofast`. `-O0`/`-O1`/`-O2` only.
- No SIMD intrinsics, no BLAS substitution. (Both belong to pass 2.)
- MPI reduction order is unchanged.

## 11. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Lambda inlining changes FP order | Keep algebraic expression identical; bit-exact gate catches it |
| nlohmann/json reads doubles differently than jansson | T10 includes parity test: dump both parsers' outputs as hex doubles, diff before changing runtime path |
| GCC 13.1 missing `<print>`/`<mdspan>` | Don't use them in pass 1; flag as pass-2 candidates |
| Coverage <100% from dead error branches | T07 deletes them; if any survive past T11, fail the ticket and reopen |
| `-Werror` flip in T12 surfaces a flood of new warnings | Land warning fixes incrementally in T05–T11 so T12 is small |

## 12. Pre-edit operating discipline

1. Work on a `pass-1` branch, never directly on `main`.
2. T02's new scaffolding lives **alongside** the legacy build, not replacing
   it, until T11.
3. Adopt `.claude/` in the repo from T04 onward.
4. Layout migration is its own dedicated ticket (T11), not folded into
   scaffolding.
5. `-Werror` is added in the closing ticket only (T12).
6. T01 demonstrably proves the bit-exact gate can fail, before any code is
   touched.

## 13. Out of scope (pass 2 candidates, recorded for later)

- Replace `Vector2D` / `Vector4D` with `std::mdspan` (libstdc++ 14+) or a
  Kokkos `mdspan` reference impl.
- Eigen / BLAS-backed contractions for the amplitude equations.
- OpenMP within MPI ranks.
- GPU offload (CUDA / SYCL / Kokkos).
- Generalize beyond HeH⁺ in STO-3G — real integral input, arbitrary basis.
- Numerical tolerance for the regression (relax bit-exactness once perf
  reorderings are intentional).
- `-O3 -march=native` profile and benchmarking harness.
