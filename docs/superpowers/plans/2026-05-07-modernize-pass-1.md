# Modernize CCSD-HeH⁺ — Pass 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Modernize an existing single-binary CCSD-for-HeH⁺ codebase for code quality only — preserving exact end-to-end energies — while putting modern C++23 tooling, layout, tests, CI, and `.claude/` discipline in place.

**Architecture:** Twelve sequential tickets on a `pass-1` branch. Each ticket is one PR. After every ticket the bit-exact regression must print `E(corr,CCSD) = -0.008225832259` and `E(CCSD) = -2.862598243` for `np ∈ {2, 4, 8}`. New scaffolding lives alongside legacy code until T11 (layout migration). `-Werror` flips on at the very end (T12).

**Tech Stack:** C++23, GCC 13.1 via `mpic++`, CMake ≥ 3.21 (presets), Ninja, Catch2 v3 via FetchContent, nlohmann/json via FetchContent, OpenMPI 3.1, Python 3 for the regression script, gcovr for coverage, clang-format / clang-tidy ≥ 14, GitHub Actions on Ubuntu 22.04, pre-commit.

**Spec:** [`docs/superpowers/specs/2026-05-07-modernize-pass-1-design.md`](../specs/2026-05-07-modernize-pass-1-design.md)

**Working branch:** `pass-1` (already created).

---

## Conventions for every task

- Every task ends with the **bit-exact regression** as its final verification step:
  ```bash
  cmake --build build && python3 tests/run_mpi_regression.py --executable build/ccsd_code
  ```
  Expected last line: `All MPI regression tests passed.`
- Every task ends with one or more `git commit` steps on the `pass-1` branch. Commits use Conventional-Commits style (`feat:`, `refactor:`, `build:`, `ci:`, `docs:`, `test:`, `chore:`). **Do not** add `Co-Authored-By` trailers (per project convention).
- "Run it to make sure it fails" is its own step before any implementation step.
- Tests use Catch2 v3. Add the test framework once (in T02), then add tests in later tasks.

---

## Task 1: T01 — Tighten the regression to bit-exact and prove it can fail

**Files:**
- Modify: `tests/run_mpi_regression.py`

This is the *first* code change because every later task depends on it being a true safety net.

- [ ] **Step 1.1: Read the current regression to confirm baseline.**

Run:
```bash
cmake -S . -B build && cmake --build build
python3 tests/run_mpi_regression.py --executable build/ccsd_code
```
Expected last line: `All MPI regression tests passed.` (currently uses `1e-9` tolerance; we'll tighten it.)

- [ ] **Step 1.2: Replace `tests/run_mpi_regression.py` with a bit-exact version.**

Write this file in full:

```python
#!/usr/bin/env python3
"""Regression test runner for the CCSD MPI executable (bit-exact mode).

The script launches the compiled binary with several MPI process counts and
asserts that two specific lines appear verbatim in stdout. This is the pass-1
contract: any change in printed digits is a code-change signal, not a
tolerance question.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

EXPECTED_LINES = (
    "  E(corr,CCSD) = -0.008225832259",
    "  E(CCSD) = -2.862598243",
)
DEFAULT_PROCESSES = (2, 4, 8)
DEFAULT_EXECUTABLE = "build/ccsd_code"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Bit-exact CCSD MPI regression."
    )
    parser.add_argument(
        "--executable",
        default=DEFAULT_EXECUTABLE,
        help=f"Path to the CCSD executable (default: {DEFAULT_EXECUTABLE})",
    )
    parser.add_argument(
        "--mpirun",
        default=shutil.which("mpirun") or "mpirun",
        help="mpirun command to use (default: first mpirun on PATH)",
    )
    parser.add_argument(
        "--np",
        nargs="+",
        type=int,
        default=list(DEFAULT_PROCESSES),
        help="List of MPI process counts to test (default: 2 4 8)",
    )
    parser.add_argument(
        "--no-oversubscribe",
        action="store_true",
        help="Do not append --oversubscribe to mpirun invocations.",
    )
    return parser.parse_args()


def run_case(args: argparse.Namespace, np: int) -> None:
    exe = Path(args.executable)
    if not exe.exists():
        raise FileNotFoundError(
            f"Executable '{exe}' not found. Build ccsd_code first."
        )

    cmd = [args.mpirun]
    if not args.no_oversubscribe:
        cmd.append("--oversubscribe")
    cmd += ["-np", str(np), str(exe)]

    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise RuntimeError(f"Command failed with exit code {result.returncode}")

    stdout_lines = result.stdout.splitlines()
    for expected in EXPECTED_LINES:
        if expected not in stdout_lines:
            sys.stderr.write(result.stdout)
            raise AssertionError(
                f"[np={np}] expected line not found verbatim:\n"
                f"  expected: {expected!r}\n"
                f"  (run with --np {np}; full stdout above)"
            )
    print(f"[np={np}] PASS  (bit-exact match)")


def main() -> None:
    args = parse_args()
    for np in args.np:
        run_case(args, np)
    print("All MPI regression tests passed.")


if __name__ == "__main__":
    main()
```

- [ ] **Step 1.3: Run it; confirm green.**

Run: `python3 tests/run_mpi_regression.py --executable build/ccsd_code`
Expected last line: `All MPI regression tests passed.`

- [ ] **Step 1.4: Prove the gate can fail.**

Make a *temporary* perturbation in `ccsd_code.cpp`. Find the energy-printing code (search for `E(corr,CCSD)` in `ccsd_code.cpp`) and change `-0.008225832259` printing to one extra space, e.g. add an extra space after the `=`. Rebuild:

```bash
cmake --build build
python3 tests/run_mpi_regression.py --executable build/ccsd_code || echo "GATE_TRIGGERED"
```
Expected: traceback ending with `AssertionError: [np=...] expected line not found verbatim`, followed by `GATE_TRIGGERED`.

- [ ] **Step 1.5: Revert the perturbation.**

Run: `git checkout -- ccsd_code.cpp && cmake --build build`
Re-run: `python3 tests/run_mpi_regression.py --executable build/ccsd_code`
Expected last line: `All MPI regression tests passed.`

- [ ] **Step 1.6: Commit.**

```bash
git add tests/run_mpi_regression.py
git commit -m "test: tighten regression to bit-exact line match"
```

---

## Task 2: T02 — Bootstrap modern build alongside legacy

**Files:**
- Create: `cmake/ProjectWarnings.cmake`
- Create: `CMakePresets.json`
- Create: `.clang-format`
- Create: `.clang-tidy`
- Create: `Makefile`
- Modify: `CMakeLists.txt` (additive — keep existing behavior; pull in warnings module; add Catch2 unit-test target stub)
- Modify: `.gitignore` (add `build/`, `build-*/`, coverage outputs)

- [ ] **Step 2.1: Create `cmake/ProjectWarnings.cmake`.**

```cmake
# Strict warning set. -Werror is intentionally NOT here yet; it is added in
# the closing pass-1 ticket (T12) once the codebase is clean.
function(project_apply_warnings TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor -Wold-style-cast
            -Wcast-align -Woverloaded-virtual -Wconversion
            -Wsign-conversion -Wnull-dereference -Wdouble-promotion
            -Wformat=2 -Wimplicit-fallthrough)
    endif()
endfunction()
```

- [ ] **Step 2.2: Create `CMakePresets.json`.**

```json
{
  "version": 3,
  "cmakeMinimumRequired": { "major": 3, "minor": 21, "patch": 0 },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "CMAKE_CXX_COMPILER": "mpicxx"
      }
    },
    {
      "name": "debug",
      "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    },
    {
      "name": "release",
      "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
    },
    {
      "name": "asan",
      "inherits": "debug",
      "cacheVariables": {
        "CCSD_ENABLE_ASAN": "ON",
        "CCSD_ENABLE_UBSAN": "ON"
      }
    },
    {
      "name": "tsan",
      "inherits": "debug",
      "cacheVariables": { "CCSD_ENABLE_TSAN": "ON" }
    },
    {
      "name": "coverage",
      "inherits": "debug",
      "cacheVariables": { "CCSD_ENABLE_COVERAGE": "ON" }
    }
  ],
  "buildPresets": [
    { "name": "debug",    "configurePreset": "debug" },
    { "name": "release",  "configurePreset": "release" },
    { "name": "asan",     "configurePreset": "asan" },
    { "name": "tsan",     "configurePreset": "tsan" },
    { "name": "coverage", "configurePreset": "coverage" }
  ],
  "testPresets": [
    { "name": "debug",    "configurePreset": "debug",    "output": { "outputOnFailure": true } },
    { "name": "asan",     "configurePreset": "asan",     "output": { "outputOnFailure": true } },
    { "name": "coverage", "configurePreset": "coverage", "output": { "outputOnFailure": true } }
  ]
}
```

- [ ] **Step 2.3: Create `.clang-format`.**

```yaml
BasedOnStyle: LLVM
Language: Cpp
Standard: c++23
ColumnLimit: 100
IndentWidth: 4
TabWidth: 4
UseTab: Never
PointerAlignment: Left
ReferenceAlignment: Left
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Attach
NamespaceIndentation: None
SortIncludes: CaseSensitive
IncludeBlocks: Regroup
SpaceAfterCStyleCast: false
SpacesBeforeTrailingComments: 2
```

- [ ] **Step 2.4: Create `.clang-tidy`.**

```yaml
Checks: >
    bugprone-*,
    modernize-*,
    performance-*,
    readability-*,
    cppcoreguidelines-*,
    -modernize-use-trailing-return-type,
    -readability-identifier-length,
    -readability-magic-numbers,
    -cppcoreguidelines-avoid-magic-numbers,
    -cppcoreguidelines-pro-bounds-pointer-arithmetic,
    -cppcoreguidelines-pro-bounds-array-to-pointer-decay,
    -cppcoreguidelines-pro-type-vararg

WarningsAsErrors: ''
HeaderFilterRegex: '^.*include/ccsd/.*\.hpp$'
FormatStyle: file
```
Note: `WarningsAsErrors: ''` until T12.

- [ ] **Step 2.5: Modify `CMakeLists.txt` — add warnings, sanitizer/coverage options, Catch2.**

Append the following block to the existing `CMakeLists.txt` (right after the `option(BUILD_TESTING ...)` line); keep the rest of the file as-is.

```cmake
# ── Pass-1 modernization additions ────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ProjectWarnings)

option(CCSD_ENABLE_ASAN     "AddressSanitizer"  OFF)
option(CCSD_ENABLE_UBSAN    "UBSanitizer"       OFF)
option(CCSD_ENABLE_TSAN     "ThreadSanitizer"   OFF)
option(CCSD_ENABLE_COVERAGE "Code coverage"     OFF)

# Apply the same flag set to ccsd_code (declared further down)
function(ccsd_apply_flags TARGET)
    project_apply_warnings(${TARGET})
    if(CCSD_ENABLE_ASAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_options   (${TARGET} PUBLIC -fsanitize=address)
    endif()
    if(CCSD_ENABLE_UBSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=undefined)
        target_link_options   (${TARGET} PUBLIC -fsanitize=undefined)
    endif()
    if(CCSD_ENABLE_TSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=thread)
        target_link_options   (${TARGET} PUBLIC -fsanitize=thread)
    endif()
    if(CCSD_ENABLE_COVERAGE)
        target_compile_options(${TARGET} PUBLIC --coverage -O0 -g)
        target_link_options   (${TARGET} PUBLIC --coverage)
    endif()
endfunction()
```

Then, **after** the existing `add_executable(ccsd_code ${SOURCES})` line, add:
```cmake
ccsd_apply_flags(ccsd_code)
```

- [ ] **Step 2.6: Wire Catch2 v3 stub via FetchContent (no real tests yet).**

Append to `CMakeLists.txt`:
```cmake
# ── Catch2 unit tests (real tests added in later tickets) ─────────────────────
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    # Smoke test: prove Catch2 is wired up. Replaced/extended in later tickets.
    add_executable(test_smoke tests/test_smoke.cpp)
    target_link_libraries(test_smoke PRIVATE Catch2::Catch2WithMain)
    ccsd_apply_flags(test_smoke)
    include(Catch)
    catch_discover_tests(test_smoke)
endif()
```

- [ ] **Step 2.7: Create `tests/test_smoke.cpp`.**

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Catch2 is wired up", "[smoke]") {
    REQUIRE(1 + 1 == 2);
}
```

- [ ] **Step 2.8: Create `Makefile` (thin wrapper).**

```makefile
.PHONY: help configure build test asan tsan coverage tidy format check regression clean

help:  ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}'

configure:  ## Configure debug build
	cmake --preset debug

build: configure  ## Build (debug)
	cmake --build --preset debug

test: build  ## Run unit tests (debug)
	ctest --preset debug

regression: build  ## Run bit-exact MPI regression
	python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code

asan:  ## ASan + UBSan
	cmake --preset asan && cmake --build --preset asan && ctest --preset asan

tsan:  ## TSan
	cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan

coverage:  ## Coverage build + gcovr
	cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage
	gcovr --root . --filter 'src/' --filter 'include/' --txt --print-summary --object-directory build/coverage || true

tidy:  ## clang-tidy on staged-able sources
	clang-tidy -p build/debug $$(git ls-files '*.cpp' '*.hpp' '*.h' | grep -v '^build')

format:  ## clang-format in place
	clang-format -i $$(git ls-files '*.cpp' '*.hpp' '*.h' | grep -v '^build')

check: format tidy test regression  ## Full quality suite

clean:  ## Remove build directories
	rm -rf build/
```

- [ ] **Step 2.9: Update `.gitignore`.**

Append:
```
# Modern build outputs
build/
build-*/
.cache/
compile_commands.json

# Coverage
*.gcov
*.gcda
*.gcno
coverage/
```

- [ ] **Step 2.10: Verify legacy build still works.**

Run: `cmake -S . -B build && cmake --build build`
Expected: builds `build/ccsd_code` cleanly (warnings now visible, but no errors).

- [ ] **Step 2.11: Verify modern build works.**

Run:
```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```
Expected: `test_smoke` passes; `ccsd_code` builds at `build/debug/ccsd_code`.

- [ ] **Step 2.12: Bit-exact regression — both build trees.**

Run:
```bash
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Expected: each ends with `All MPI regression tests passed.`

- [ ] **Step 2.13: Commit.**

```bash
git add cmake/ProjectWarnings.cmake CMakePresets.json .clang-format .clang-tidy \
        Makefile CMakeLists.txt tests/test_smoke.cpp .gitignore
git commit -m "build: add CMake presets, warnings module, Catch2 stub, Makefile wrapper"
```

---

## Task 3: T03 — GitHub Actions CI

**Files:**
- Create: `.github/workflows/ci.yml`

- [ ] **Step 3.1: Create the workflow.**

```yaml
name: CI

on:
  push:
    branches: [main, pass-1]
  pull_request:
    branches: [main]

jobs:
  debug:
    name: Debug build + unit tests + regression
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-13 libopenmpi-dev openmpi-bin \
                                  libjansson-dev ninja-build cmake python3
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
      - name: Configure
        run: cmake --preset debug
      - name: Build
        run: cmake --build --preset debug
      - name: Unit tests
        run: ctest --preset debug
      - name: Bit-exact regression
        run: python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code

  asan:
    name: ASan + UBSan + regression
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-13 libopenmpi-dev openmpi-bin \
                                  libjansson-dev ninja-build cmake python3
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
      - name: Configure
        run: cmake --preset asan
      - name: Build
        run: cmake --build --preset asan
      - name: Unit tests
        run: ctest --preset asan
      - name: Bit-exact regression (np=2 only under sanitizers)
        run: python3 tests/run_mpi_regression.py --executable build/asan/ccsd_code --np 2

  coverage:
    name: Coverage
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-13 libopenmpi-dev openmpi-bin \
                                  libjansson-dev ninja-build cmake python3 gcovr
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
      - name: Configure
        run: cmake --preset coverage
      - name: Build
        run: cmake --build --preset coverage
      - name: Unit tests
        run: ctest --preset coverage
      - name: Regression run for coverage
        run: python3 tests/run_mpi_regression.py --executable build/coverage/ccsd_code --np 4
      - name: Coverage report (warn-only until T11)
        run: |
          gcovr --root . --filter 'src/' --filter 'include/' --txt \
                --print-summary --object-directory build/coverage || true
```

- [ ] **Step 3.2: Verify locally where possible.**

Run: `python3 -c 'import yaml; yaml.safe_load(open(".github/workflows/ci.yml"))'`
Expected: no output (YAML valid). If the box has no PyYAML, install via `pip install --user pyyaml` first.

- [ ] **Step 3.3: Commit and push.**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: add GitHub Actions debug/asan/coverage jobs"
git push -u origin pass-1
```

After push, verify the workflow runs green on GitHub. If a job fails, fix in this ticket and amend; do not merge until all three jobs are green.

---

## Task 4: T04 — Per-project `.claude/` and pre-commit hooks

**Files:**
- Create: `.claude/CLAUDE.md`
- Create: `.claude/agents/ccsd-reviewer.md`
- Modify: `.gitignore` (add `.claude/settings.local.json`)
- Create: `.pre-commit-config.yaml`

- [ ] **Step 4.1: Create `.claude/CLAUDE.md`.**

```markdown
# Project: ccsd-hehp-cpp

## Hard invariant
Every commit must keep the bit-exact regression green for `np ∈ {2, 4, 8}`:
```
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Last line must be: `All MPI regression tests passed.`

## Toolchain
- C++23, GCC 13.1 via `mpicxx`
- CMake presets: `debug`, `release`, `asan`, `tsan`, `coverage`
- Build: `cmake --preset debug && cmake --build --preset debug`
- Tests: `ctest --preset debug`
- Regression: `make regression`
- Full check: `make check`

## Rules (pass 1 of modernization)
1. Never modify the bit-exact regression script to make a refactor pass.
   Changing FP order is a real change — fix the refactor, not the gate.
2. No SIMD, no BLAS swaps, no OpenMP, no `-ffast-math`. Pass 2 territory.
3. Stay on `pass-1` branch. One ticket per PR.
4. -Werror is OFF until ticket T12. Warnings appear; only fix them in the
   ticket that owns that code.
5. Never use `--no-verify` to bypass pre-commit hooks.

## Authorship
- Author = Mehdi Jenab.
- No `Co-Authored-By` trailers in commit messages.
```

- [ ] **Step 4.2: Create `.claude/agents/ccsd-reviewer.md`.**

```markdown
---
name: ccsd-reviewer
description: "Reviews pass-1 modernization PRs. Use when reviewing a ticket PR or before merging into main. Verifies bit-exact regression, ticket-scope discipline, and the no-numerical-drift contract."
---

# CCSD Pass-1 Reviewer

Checklist for any PR on the `pass-1` branch:

1. **Bit-exact gate green:** `python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code` ends with `All MPI regression tests passed.`
2. **Scope discipline:** the diff matches exactly one ticket from `docs/superpowers/plans/2026-05-07-modernize-pass-1.md`. No "while I was there" cleanup.
3. **No determinism breakers:** no SIMD, no `-ffast-math`/`-Ofast`, no reassociated sums, no MPI reduction order changes.
4. **Compile flags unchanged unless this ticket owns them.** Only T02 and T12 touch `cmake/ProjectWarnings.cmake`.
5. **Tests added where the ticket is supposed to add tests** (T07, T10, T11 in particular).

Report: list of violations (if any), or "LGTM" + one-line summary.
```

- [ ] **Step 4.3: Update `.gitignore` to exclude local Claude settings.**

Append:
```
# Per-project Claude local config (machine-specific)
.claude/settings.local.json
```

- [ ] **Step 4.4: Create `.pre-commit-config.yaml`.**

```yaml
repos:
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v17.0.6
    hooks:
      - id: clang-format
        types_or: [c++, c]

  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.6.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
```

Note: clang-tidy and a custom regression hook are added in T12, not here.

- [ ] **Step 4.5: Install hooks.**

Run:
```bash
pip install --user pre-commit
pre-commit install
```
Expected: `pre-commit installed at .git/hooks/pre-commit`.

- [ ] **Step 4.6: Run pre-commit on all files; expect a one-shot reformat.**

Run: `pre-commit run --all-files`
Expected: clang-format may rewrite `*.cpp`/`*.h`. Other hooks may fix trailing whitespace.

- [ ] **Step 4.7: Verify the reformatted code still produces bit-exact energies.**

Run:
```bash
cmake --build --preset debug
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Expected last line: `All MPI regression tests passed.`

If this fails: revert (`git checkout -- ccsd_code.cpp ParameterClass.h ParameterClass_NoJson.h MpiClass.h VectorsClass.h`) and investigate. clang-format should not be able to break numerics, but in pathological cases (e.g. a pre-existing `#define` that depends on layout) it can — fix by widening `.clang-format` exclusions.

- [ ] **Step 4.8: Commit reformat + Claude config in two commits.**

```bash
# First, commit the mechanical reformat alone so blame remains useful.
git add ccsd_code.cpp ParameterClass.h ParameterClass_NoJson.h MpiClass.h VectorsClass.h
git commit -m "style: clang-format reformat (no semantic changes)"

# Then, commit the new config files.
git add .claude/CLAUDE.md .claude/agents/ccsd-reviewer.md \
        .pre-commit-config.yaml .gitignore
git commit -m "chore: add .claude config and pre-commit hooks"
```

---

## Task 5: T05 — Drop `using namespace std` from headers; remove `ParameterClass_NoJson.h`

**Files:**
- Modify: `ParameterClass.h`
- Modify: `VectorsClass.h`
- Modify: `MpiClass.h` (no `using namespace std` to remove, but verify)
- Delete: `ParameterClass_NoJson.h`
- Modify: `ccsd_code.cpp` (remove the `// #include "ParameterClass_NoJson.h"` comment block)
- Modify: `CMakeLists.txt` (remove `ParameterClass_NoJson.h` from `HEADERS`)

- [ ] **Step 5.1: Remove `using namespace std;` from `ParameterClass.h`.**

Edit `ParameterClass.h`:
- Delete the line `using namespace std;` near the top.
- Replace each unqualified `cout`, `cerr`, `endl`, `array`, `map`, `pair` inside the header with `std::cout`, `std::cerr`, `std::endl`, `std::array`, `std::map`, `std::pair`. The `std::array<...>` and `std::map<...>` declarations already use the qualified form; only the bare `cout`/`cerr` calls and the `endl` in error messages need fixing.

- [ ] **Step 5.2: Remove `using namespace std;` from `VectorsClass.h` (if present), qualify `std::cout`, `std::endl`, `std::ostream`, `std::vector`, `std::size_t`, `std::fill`.**

Search:
```bash
grep -n 'using namespace std' VectorsClass.h
```
If present, delete that line and qualify the remaining unqualified `cout`/`endl`/`ostream`/`vector`/`fill`/`size_t` references.

- [ ] **Step 5.3: Verify `MpiClass.h` is clean.**

Run: `grep -n 'using namespace' MpiClass.h`
Expected: no output.

- [ ] **Step 5.4: Delete `ParameterClass_NoJson.h`.**

```bash
git rm ParameterClass_NoJson.h
```

- [ ] **Step 5.5: Remove the dead `#include` comment in `ccsd_code.cpp`.**

Edit `ccsd_code.cpp` near the top:
- Delete the two-line block:
  ```cpp
  // #include "ParameterClass_NoJson.h"
  // in case, one wants to avoid using JSON file format use this instead of "ParameterClass.h"
  ```

- [ ] **Step 5.6: Update `CMakeLists.txt` HEADERS list.**

Edit `CMakeLists.txt`. Find:
```cmake
set(HEADERS
    ParameterClass.h
    ParameterClass_NoJson.h
    MpiClass.h
    VectorsClass.h
)
```
Replace with:
```cmake
set(HEADERS
    ParameterClass.h
    MpiClass.h
    VectorsClass.h
)
```

- [ ] **Step 5.7: Build both trees.**

```bash
cmake --build build              # legacy
cmake --build --preset debug     # modern
```
Both expected to compile cleanly.

- [ ] **Step 5.8: Bit-exact regression.**

```bash
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Both expected: `All MPI regression tests passed.`

- [ ] **Step 5.9: Commit.**

```bash
git add ParameterClass.h VectorsClass.h ccsd_code.cpp CMakeLists.txt
git rm ParameterClass_NoJson.h
git commit -m "refactor: drop \`using namespace std\` from headers; remove unused NoJson variant"
```

---

## Task 6: T06 — RAII `MpiSession`

**Files:**
- Modify: `MpiClass.h` → keep filename for now (rename happens in T11), but rewrite contents
- Modify: `ccsd_code.cpp`

- [ ] **Step 6.1: Rewrite `MpiClass.h` with RAII semantics.**

```cpp
#ifndef MpiClass_Included
#define MpiClass_Included

#include <mpi.h>

namespace ccsd {

class MpiSession {
public:
    MpiSession(int* argc, char*** argv) {
        MPI_Init(argc, argv);
        MPI_Comm_size(MPI_COMM_WORLD, &size_);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    }

    ~MpiSession() { MPI_Finalize(); }

    MpiSession(const MpiSession&) = delete;
    MpiSession& operator=(const MpiSession&) = delete;
    MpiSession(MpiSession&&) = delete;
    MpiSession& operator=(MpiSession&&) = delete;

    [[nodiscard]] int rank() const noexcept { return rank_; }
    [[nodiscard]] int size() const noexcept { return size_; }

private:
    int rank_ = 0;
    int size_ = 0;
};

}  // namespace ccsd

// Backwards-compat shim used by ccsd_code.cpp until globals are removed in T09.
// Kept deliberately small.
class MpiClass {
public:
    int size = 0;
    int rank = 0;
};

#endif  // MpiClass_Included
```

- [ ] **Step 6.2: Update `ccsd_code.cpp` to use `MpiSession`.**

Find the lines (top of file and inside `main`):
```cpp
MpiClass  mpi;
...
mpi.Initialize_MPI();
...
mpi.Finalize_MPI();
```

Replace the global `MpiClass mpi;` with the same `MpiClass mpi;` (kept as data carrier) and rewrite `main()` to:
```cpp
int main(int argc, char** argv) {
    ccsd::MpiSession session(&argc, &argv);
    mpi.size = session.size();
    mpi.rank = session.rank();

    // ... existing body ...
    // (remove the explicit mpi.Initialize_MPI() and mpi.Finalize_MPI() calls)
    return 0;
}
```

The `MpiClass mpi` global stays for now — T09 deletes it along with all the others.

- [ ] **Step 6.3: Build both trees.**

```bash
cmake --build build
cmake --build --preset debug
```

- [ ] **Step 6.4: Bit-exact regression.**

```bash
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Both: `All MPI regression tests passed.`

- [ ] **Step 6.5: Sanity check that only the new TU manages MPI lifetime.**

```bash
grep -nE 'MPI_Init|MPI_Finalize' *.cpp *.h
```
Expected: only matches inside `MpiClass.h`'s `MpiSession` ctor/dtor.

- [ ] **Step 6.6: Commit.**

```bash
git add MpiClass.h ccsd_code.cpp
git commit -m "refactor: RAII MpiSession; only ctor/dtor own MPI_Init/Finalize"
```

---

## Task 7: T07 — Fix tensor leaf API; add Catch2 unit tests

**Files:**
- Modify: `VectorsClass.h`
- Create: `include/ccsd/mpi_tensor.hpp` (free-function MPI helpers — interim header; absorbed in T11)
- Modify: `ccsd_code.cpp` (callsites: `*v.set(i,j) = x` → `v(i,j) = x`)
- Create: `tests/test_tensors.cpp`
- Modify: `CMakeLists.txt` (replace `test_smoke` with `test_tensors`, add include dir)

- [ ] **Step 7.1: Rewrite `VectorsClass.h`.**

The new file replaces both `Vector2D` and `Vector4D` with stricter, MPI-free versions. Free MPI helpers live in a new header.

```cpp
#ifndef VectorsClass_Included
#define VectorsClass_Included  // typo fixed (was VectorsClas_Included)

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

namespace ccsd {

class Vector2D {
public:
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

    [[nodiscard]] double  operator()(int i, int j) const {
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
    [[nodiscard]] double*       raw()       noexcept { return data_.data(); }
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
    Vector4D() = default;

    void initialization(int dim2) {
        n1_ = n2_ = n3_ = n4_ = dim2;
        n_size_ = n1_ * n2_ * n3_ * n4_;
        data_.assign(static_cast<std::size_t>(n_size_), 0.0);
    }

    void zeros() { std::fill(data_.begin(), data_.end(), 0.0); }

    [[nodiscard]] double  operator()(int i, int j, int k, int l) const {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_ && k >= 0 && k < n3_ && l >= 0 && l < n4_);
        return data_[index(i, j, k, l)];
    }
    [[nodiscard]] double& operator()(int i, int j, int k, int l) {
        assert(i >= 0 && i < n1_ && j >= 0 && j < n2_ && k >= 0 && k < n3_ && l >= 0 && l < n4_);
        return data_[index(i, j, k, l)];
    }

    [[nodiscard]] int n_size() const noexcept { return n_size_; }
    [[nodiscard]] double*       raw()       noexcept { return data_.data(); }
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

}  // namespace ccsd

// Bring symbols into the global namespace temporarily so ccsd_code.cpp's
// existing free functions don't all need re-qualification in this ticket.
// Removed in T09/T11.
using Vector2D = ccsd::Vector2D;
using Vector4D = ccsd::Vector4D;

#endif  // VectorsClass_Included
```

**Important:** the index encoding must remain identical to the old code. The old encoding was:
- 2D: `index = j*n1 + i`  → matches `index(i,j)` above.
- 4D: `index = l*N1*N2*N3 + k*N1*N2 + j*N1 + i` → matches `index(i,j,k,l)` above.

This preserves the memory layout exactly, which preserves how `MPI_Send`/`MPI_Bcast` interpret the buffer.

- [ ] **Step 7.2: Create `include/ccsd/mpi_tensor.hpp`.**

```cpp
#pragma once
#include <mpi.h>
#include "../../VectorsClass.h"  // adjusted in T11 to <ccsd/tensors.hpp>

namespace ccsd::mpi {

inline void send(Vector2D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, 233, MPI_COMM_WORLD);
}
inline void recv(Vector2D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, 233, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector2D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

inline void send(Vector4D& t, int dst) {
    MPI_Send(t.raw(), t.n_size(), MPI_DOUBLE, dst, 610, MPI_COMM_WORLD);
}
inline void recv(Vector4D& t, int src) {
    MPI_Recv(t.raw(), t.n_size(), MPI_DOUBLE, src, 610, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
inline void bcast(Vector4D& t, int src) {
    MPI_Bcast(t.raw(), t.n_size(), MPI_DOUBLE, src, MPI_COMM_WORLD);
}

}  // namespace ccsd::mpi
```

The MPI tags `233` (2D) and `610` (4D) are kept identical to the old code so wire-format compatibility is preserved across the refactor.

- [ ] **Step 7.3: Update `ccsd_code.cpp` callsites.**

Mechanical changes — search-replace patterns:
- `*<X>.set(i,j) = ` → `<X>(i,j) = `
- `*<X>.set(i,j,k,l) = ` → `<X>(i,j,k,l) = `
- `*<X>.set(i,j) +=` → `<X>(i,j) +=`
- `*<X>.set(i,j,k,l) +=` → `<X>(i,j,k,l) +=`
- `<X>.send(rank)` → `ccsd::mpi::send(<X>, rank)`
- `<X>.recv(rank)` → `ccsd::mpi::recv(<X>, rank)`
- `<X>.mpi_bcast(rank)` → `ccsd::mpi::bcast(<X>, rank)`
- `<X>.zeros()` — keep as-is (still a member).

Also add `#include "include/ccsd/mpi_tensor.hpp"` near the top of `ccsd_code.cpp`.

Run after editing:
```bash
grep -nE '\.set\(' ccsd_code.cpp || echo "OK no .set() callsites"
grep -nE '\.(send|recv|mpi_bcast)\(' ccsd_code.cpp || echo "OK no member MPI ops"
```
Expected: both echo their `OK` lines.

- [ ] **Step 7.4: Write `tests/test_tensors.cpp`.**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "../VectorsClass.h"

TEST_CASE("Vector2D index encoding matches legacy layout", "[tensor][2d]") {
    ccsd::Vector2D v;
    int dim = 4;
    v.initialization(dim);

    for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j)
            v(i, j) = static_cast<double>(j * dim + i);  // matches old j*n1 + i

    REQUIRE(v.raw()[0] == 0.0);                        // (0,0)
    REQUIRE(v.raw()[1] == 1.0);                        // (1,0)  -> j=0, i=1
    REQUIRE(v.raw()[dim] == static_cast<double>(dim)); // (0,1)  -> j=1, i=0

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
            t(i, j, k, l) = static_cast<double>(
                l * dim*dim*dim + k * dim*dim + j * dim + i);

    REQUIRE(t.raw()[0]                 == 0.0);                                // (0,0,0,0)
    REQUIRE(t.raw()[1]                 == 1.0);                                // (1,0,0,0)
    REQUIRE(t.raw()[dim]               == static_cast<double>(dim));           // (0,1,0,0)
    REQUIRE(t.raw()[dim*dim]           == static_cast<double>(dim*dim));       // (0,0,1,0)
    REQUIRE(t.raw()[dim*dim*dim]       == static_cast<double>(dim*dim*dim));   // (0,0,0,1)
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
```

- [ ] **Step 7.5: Replace the `test_smoke` target with `test_tensors` in `CMakeLists.txt`.**

Edit `CMakeLists.txt`. Find:
```cmake
    add_executable(test_smoke tests/test_smoke.cpp)
    target_link_libraries(test_smoke PRIVATE Catch2::Catch2WithMain)
    ccsd_apply_flags(test_smoke)
    include(Catch)
    catch_discover_tests(test_smoke)
```
Replace with:
```cmake
    add_executable(test_tensors tests/test_tensors.cpp)
    target_link_libraries(test_tensors PRIVATE Catch2::Catch2WithMain)
    target_include_directories(test_tensors PRIVATE ${CMAKE_SOURCE_DIR})
    ccsd_apply_flags(test_tensors)
    include(Catch)
    catch_discover_tests(test_tensors)
```

Then `git rm tests/test_smoke.cpp`.

- [ ] **Step 7.6: Run unit tests.**

```bash
cmake --build --preset debug
ctest --preset debug
```
Expected: 4 passing test cases.

- [ ] **Step 7.7: Bit-exact regression.**

```bash
cmake --build build
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Both: `All MPI regression tests passed.`

- [ ] **Step 7.8: Commit.**

```bash
git add VectorsClass.h include/ccsd/mpi_tensor.hpp ccsd_code.cpp \
        tests/test_tensors.cpp CMakeLists.txt
git rm tests/test_smoke.cpp
git commit -m "refactor: tighten Vector2D/4D API; decouple MPI; add Catch2 tensor tests"
```

---

## Task 8: T08 — Replace macro loop bundles with named iteration helpers

**Files:**
- Modify: `ccsd_code.cpp`

The current code uses macros like:
```cpp
#define iLoop for(int i=0;i<p.Nelec;++i)
#define aLoop for(int a=p.Nelec;a<dim2;++a)
```
We replace them with two free functions.

- [ ] **Step 8.1: Add helper at the top of `ccsd_code.cpp` (after the includes).**

```cpp
namespace {

template <class F> inline void for_occ(int Nelec, F&& f) {
    for (int idx = 0; idx < Nelec; ++idx) f(idx);
}
template <class F> inline void for_virt(int Nelec, int dim2, F&& f) {
    for (int idx = Nelec; idx < dim2; ++idx) f(idx);
}

}  // namespace
```

- [ ] **Step 8.2: Delete the macro defines.**

Remove the eight `#define` lines (`mLoop`, `nLoop`, `iLoop`, `jLoop`, `eLoop`, `fLoop`, `aLoop`, `bLoop`).

- [ ] **Step 8.3: Convert each loop site mechanically.**

For each site, replace the bare loop nest with explicit lambdas. Pattern:

```cpp
// Before:
aLoop {
    bLoop {
        iLoop {
            jLoop {
                *td.set(a,b,i,j) += spinints(i,j,a,b)/(fs(i,i)+fs(j,j)-fs(a,a)-fs(b,b));
            }
        }
    }
}

// After:
for_virt(p.Nelec, dim2, [&](int a){
  for_virt(p.Nelec, dim2, [&](int b){
    for_occ(p.Nelec, [&](int i){
      for_occ(p.Nelec, [&](int j){
        td(a,b,i,j) += spinints(i,j,a,b)/(fs(i,i)+fs(j,j)-fs(a,a)-fs(b,b));
      });
    });
  });
});
```

Apply this transformation to every macro-loop site. The expression inside the innermost block must be **byte-identical** to the original to preserve FP order.

- [ ] **Step 8.4: Verify no macro defs remain.**

```bash
grep -nE '#define [ijmnefab]Loop' ccsd_code.cpp
```
Expected: no output.

- [ ] **Step 8.5: Build and bit-exact regression.**

```bash
cmake --build build
cmake --build --preset debug
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Both: `All MPI regression tests passed.`

If the regression fails: the most likely cause is that the lambda capture changed evaluation order of subexpressions in a sum. Compare the offending function's diff to the original; the right inside the innermost `{}` must be exactly the original. Fix and re-run.

- [ ] **Step 8.6: Commit.**

```bash
git add ccsd_code.cpp
git commit -m "refactor: replace macro loop bundles with for_occ/for_virt lambdas"
```

---

## Task 9: T09 — Eliminate file-scope globals; introduce `CcsdSolver`

**Files:**
- Modify: `ccsd_code.cpp`
- (Optional) introduce a transitional internal header `ccsd_solver.hpp` co-located with `ccsd_code.cpp`. T11 moves it under `include/ccsd/`.

This is the largest pass-1 ticket. The discipline is: keep every loop body **byte-identical**, only move them into methods of a class that owns the state.

- [ ] **Step 9.1: Identify every file-scope global.**

Run:
```bash
grep -nE '^(int |Vector2D |Vector4D |MpiClass |ParameterClass )' ccsd_code.cpp
```
Expected globals (verify against output): `ParameterClass p`, `MpiClass mpi`, `int rank_master, rank_start`, `Vector2D Fae, Fmi, Fme, Dai, tsnew, ts, fs`, `Vector4D Wmnij, Wabef, Wmbej, Dabij, tdnew, td, spinints`, `int dim2`.

- [ ] **Step 9.2: Create the solver class at the top of `ccsd_code.cpp`.**

```cpp
class CcsdSolver {
public:
    CcsdSolver(const ParameterClass& params, int rank, int size)
        : p_(params), rank_(rank), size_(size) {
        dim2_ = p_.dim * 2;
        Fae_.initialization(dim2_);
        Fmi_.initialization(dim2_);
        Fme_.initialization(dim2_);
        Dai_.initialization(dim2_);
        tsnew_.initialization(dim2_);
        ts_.initialization(dim2_);
        fs_.initialization(dim2_);
        Wmnij_.initialization(dim2_);
        Wabef_.initialization(dim2_);
        Wmbej_.initialization(dim2_);
        Dabij_.initialization(dim2_);
        tdnew_.initialization(dim2_);
        td_.initialization(dim2_);
        spinints_.initialization(dim2_);
    }

    void run();  // top-level driver: calls everything and prints energies

private:
    // Move every existing free function in ccsd_code.cpp into a method here:
    //   guess_T2, get_denominator_arrays, get_key, get_spinints,
    //   make_intermediate_arrays, makeT1, makeT2, ccsdEnergy, etc.
    // Method bodies are byte-identical to the original; only the data
    // accesses change from `td`/`fs`/... to `td_`/`fs_`/... and `p.Nelec`
    // becomes `p_.Nelec`.

    const ParameterClass& p_;
    int rank_;
    int size_;
    int dim2_ = 0;

    Vector2D Fae_, Fmi_, Fme_, Dai_, tsnew_, ts_, fs_;
    Vector4D Wmnij_, Wabef_, Wmbej_, Dabij_, tdnew_, td_, spinints_;
};
```

- [ ] **Step 9.3: Move each existing free function into the class.**

For each function (`guess_T2`, `get_denominator_arrays`, `get_key`, `get_spinints`, `make_intermediate_arrays`, `makeT1`, `makeT2`, `ccsdEnergy`, etc. — discover via `grep -nE '^[a-zA-Z]+ [a-zA-Z_]+\\(' ccsd_code.cpp`):
1. Move its declaration into the `CcsdSolver` class as a private method.
2. Move its body into the same file outside the class as `ReturnType CcsdSolver::method_name(...) { ... }`.
3. Rewrite each global access to its underscored member equivalent (`td` → `td_`, `fs` → `fs_`, `p.Nelec` → `p_.Nelec`, `dim2` → `dim2_`, `mpi.rank` → `rank_`, `mpi.size` → `size_`).

Inside lambda captures from T08, change `[&]` to still capture by reference; `this` is captured automatically when calling member data from a member-function lambda. **Keep every arithmetic expression byte-identical.**

- [ ] **Step 9.4: Implement `CcsdSolver::run()` as the orchestrator.**

It contains the body of what used to be `int main()` minus MPI init/finalize. The order of method calls must exactly mirror the original `main()` ordering. The print statements at the end (`E(corr,CCSD)` and `E(CCSD)`) must still produce the exact two lines.

- [ ] **Step 9.5: Reduce `main()` to a thin shell.**

```cpp
int main(int argc, char** argv) {
    ccsd::MpiSession session(&argc, &argv);
    ParameterClass params;  // reads config.json
    CcsdSolver solver(params, session.rank(), session.size());
    solver.run();
    return 0;
}
```

- [ ] **Step 9.6: Delete the file-scope globals.**

Confirm no remaining definitions:
```bash
grep -nE '^(int |Vector2D |Vector4D |MpiClass |ParameterClass )' ccsd_code.cpp
```
Expected: only matches inside class bodies (none at column 0).

- [ ] **Step 9.7: Build and run unit tests.**

```bash
cmake --build build
cmake --build --preset debug
ctest --preset debug
```
Expected: tensor tests still pass.

- [ ] **Step 9.8: Bit-exact regression.**

```bash
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Both: `All MPI regression tests passed.`

If a regression fails, **bisect** by comparing the original `ccsd_code.cpp` to the new one method-by-method. The most likely cause is a missed expression rewrite (e.g. `fs(a,a)` accidentally became `fs_(a, a)` with a stray space — irrelevant — or the order of two terms in a sum got swapped — fatal).

- [ ] **Step 9.9: Commit.**

```bash
git add ccsd_code.cpp
git commit -m "refactor: introduce CcsdSolver; eliminate file-scope globals"
```

---

## Task 10: T10 — Switch `ParameterClass` to nlohmann/json with name-keyed parsing

**Files:**
- Modify: `CMakeLists.txt` (FetchContent nlohmann/json; drop jansson find)
- Modify: `ParameterClass.h` (full rewrite)
- Modify: `config.json` (verify schema is name-keyed, not array-keyed; the existing file is already a `config` array — convert to a flat object)
- Modify: `ccsd_code.cpp` (no API change — `ParameterClass p; p.dim;` still works)
- Modify: `tests/test_parameters.cpp` (new)

- [ ] **Step 10.1: Inspect current `config.json` and decide schema.**

```bash
python3 -c 'import json; print(json.dumps(json.load(open("config.json")), indent=2)[:600])'
```
Confirm: top-level key `config` is an array of objects keyed by index. The new schema is a flat object:
```json
{
  "dim": 2,
  "Nelec": 2,
  "orbital_energy": [..., ...],
  "ENUC": ...,
  "EN": ...,
  "ttmo": [k0, v0, k1, v1, ...]
}
```

- [ ] **Step 10.2: Write a one-shot Python migration script and run it once.**

Create `tools/migrate_config.py`:
```python
import json, sys
src = json.load(open(sys.argv[1]))
arr = src["config"]
out = {}
for entry in arr:
    out.update(entry)
json.dump(out, open(sys.argv[1], "w"), indent=2)
```
Run: `python3 tools/migrate_config.py config.json`
Verify: `python3 -c 'import json; d=json.load(open("config.json")); print(sorted(d))'`
Expected: `['EN', 'ENUC', 'Nelec', 'dim', 'orbital_energy', 'ttmo']`

Keep `tools/migrate_config.py` in the repo for reproducibility.

- [ ] **Step 10.3: Update `CMakeLists.txt` — FetchContent nlohmann/json; drop jansson.**

Find and remove:
```cmake
if(USE_JSON)
    find_library(JANSSON_LIBRARY jansson)
    ...
endif()
```
And:
```cmake
if(USE_JSON AND JANSSON_LIBRARY)
    target_link_libraries(ccsd_code PRIVATE ${JANSSON_LIBRARY})
    target_compile_definitions(ccsd_code PRIVATE USE_JSON)
endif()
```

Add (next to the existing Catch2 FetchContent block):
```cmake
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(ccsd_code PRIVATE nlohmann_json::nlohmann_json)
```

Remove the `option(USE_JSON ...)` line — JSON is now mandatory.

- [ ] **Step 10.4: Rewrite `ParameterClass.h`.**

```cpp
#ifndef ParameterClass_Included
#define ParameterClass_Included

#include <array>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

class ParameterClass {
public:
    int dim = 0;
    int Nelec = 0;
    std::array<double, 2> orbital_energy{};
    double ENUC = 0.0;
    double EN = 0.0;
    std::map<double, double> ttmo;

    explicit ParameterClass(const std::string& path = "./config.json") {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open " + path);
        nlohmann::json j;
        in >> j;

        dim   = j.at("dim").get<int>();
        Nelec = j.at("Nelec").get<int>();

        const auto& oe = j.at("orbital_energy");
        if (oe.size() != 2) throw std::runtime_error("orbital_energy must have 2 entries");
        orbital_energy[0] = oe[0].get<double>();
        orbital_energy[1] = oe[1].get<double>();

        ENUC = j.at("ENUC").get<double>();
        EN   = j.at("EN").get<double>();

        const auto& flat = j.at("ttmo");
        for (std::size_t i = 0; i + 1 < flat.size(); i += 2) {
            ttmo.emplace(flat[i].get<double>(), flat[i + 1].get<double>());
        }
    }
};

#endif  // ParameterClass_Included
```

- [ ] **Step 10.5: Verify `ccsd_code.cpp` callsites still compile.**

The public API of `ParameterClass` is unchanged (`p.dim`, `p.Nelec`, `p.orbital_energy`, `p.ENUC`, `p.EN`, `p.ttmo`). No edits required to the solver.

- [ ] **Step 10.6: Drop the now-stale `#include <jansson.h>` from `ccsd_code.cpp`.**

Find and delete `#include <jansson.h>`.

- [ ] **Step 10.7: Add unit test `tests/test_parameters.cpp`.**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <fstream>

#include "../ParameterClass.h"

namespace {
const char* kFixture = R"({
  "dim": 2,
  "Nelec": 2,
  "orbital_energy": [-0.913, 1.395],
  "ENUC": 1.300,
  "EN": -2.854,
  "ttmo": [0.5, 0.31, 1.5, 0.42]
})";
}  // namespace

TEST_CASE("ParameterClass parses a name-keyed config", "[parameters]") {
    const std::string path = "test_config_tmp.json";
    {
        std::ofstream out(path);
        out << kFixture;
    }
    ParameterClass p(path);
    REQUIRE(p.dim == 2);
    REQUIRE(p.Nelec == 2);
    REQUIRE(p.orbital_energy[0] == -0.913);
    REQUIRE(p.orbital_energy[1] ==  1.395);
    REQUIRE(p.ENUC ==  1.300);
    REQUIRE(p.EN   == -2.854);
    REQUIRE(p.ttmo.at(0.5) == 0.31);
    REQUIRE(p.ttmo.at(1.5) == 0.42);
    std::remove(path.c_str());
}
```

- [ ] **Step 10.8: Wire `test_parameters` into CMake.**

In `CMakeLists.txt`, alongside `test_tensors`:
```cmake
add_executable(test_parameters tests/test_parameters.cpp)
target_link_libraries(test_parameters PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_include_directories(test_parameters PRIVATE ${CMAKE_SOURCE_DIR})
ccsd_apply_flags(test_parameters)
catch_discover_tests(test_parameters)
```

- [ ] **Step 10.9: Build and run.**

```bash
cmake --build build
cmake --build --preset debug
ctest --preset debug
python3 tests/run_mpi_regression.py --executable build/ccsd_code
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
All four expected green.

If `ttmo` parity is suspected: write a one-off diff script that compares the new map's `(key, value)` pairs against a saved gold copy of the old jansson map. Bit-exactness of the regression is the final arbiter.

- [ ] **Step 10.10: Commit.**

```bash
git add CMakeLists.txt ParameterClass.h ccsd_code.cpp config.json \
        tools/migrate_config.py tests/test_parameters.cpp
git commit -m "build: replace jansson with nlohmann/json; name-keyed config schema"
```

---

## Task 11: T11 — Migrate to `include/ccsd/` + `src/` + `apps/` layout

**Files:**
- Move: `VectorsClass.h` → `include/ccsd/tensors.hpp`
- Move: `ParameterClass.h` → `include/ccsd/parameters.hpp`
- Move: `MpiClass.h` → `include/ccsd/mpi_session.hpp`
- Move: `include/ccsd/mpi_tensor.hpp` (already exists from T07; update its include path)
- Split: `ccsd_code.cpp` → `apps/ccsd_code.cpp` (thin main) + `src/{driver,intermediates,amplitudes,energy}.cpp` + matching headers in `include/ccsd/`
- Replace: `CMakeLists.txt`, `tests/CMakeLists.txt`, add `src/CMakeLists.txt`
- Update: `tests/test_tensors.cpp`, `tests/test_parameters.cpp` includes

The mechanical moves are the easy part; the file split is delicate because of FP ordering. We keep every expression in its original loop body verbatim.

- [ ] **Step 11.1: Create the new directory tree.**

```bash
mkdir -p include/ccsd src apps
```

- [ ] **Step 11.2: Move and rename headers.**

```bash
git mv VectorsClass.h    include/ccsd/tensors.hpp
git mv ParameterClass.h  include/ccsd/parameters.hpp
git mv MpiClass.h        include/ccsd/mpi_session.hpp
```

In each moved file, update header guards: `tensors.hpp` → `CCSD_TENSORS_HPP_INCLUDED`, etc. Replace the `using` aliases at the bottom of `tensors.hpp` (the temporary global-namespace shims from T07) with — nothing, deleted. Inside the new headers, all types live in `namespace ccsd { ... }` (this is already the case after T07).

- [ ] **Step 11.3: Update `include/ccsd/mpi_tensor.hpp` to include the new tensor header.**

Change `#include "../../VectorsClass.h"` → `#include <ccsd/tensors.hpp>`.

- [ ] **Step 11.4: Split the body of `ccsd_code.cpp` into source files.**

Create the following with the methods of `CcsdSolver` introduced in T09:

- `include/ccsd/driver.hpp` — declaration of `class ccsd::CcsdSolver` and `void run()`.
- `src/driver.cpp` — `CcsdSolver` ctor and `run()`.
- `include/ccsd/intermediates.hpp` + `src/intermediates.cpp` — `make_intermediate_arrays`, `get_denominator_arrays`, helpers like `get_key`, `get_spinints`.
- `include/ccsd/amplitudes.hpp` + `src/amplitudes.cpp` — `guess_T2`, `makeT1`, `makeT2`, `update_amplitudes`.
- `include/ccsd/energy.hpp` + `src/energy.cpp` — `ccsdEnergy` (or whatever the energy function is called).
- `apps/ccsd_code.cpp` — only:
  ```cpp
  #include <ccsd/driver.hpp>
  #include <ccsd/mpi_session.hpp>
  #include <ccsd/parameters.hpp>

  int main(int argc, char** argv) {
      ccsd::MpiSession session(&argc, &argv);
      ccsd::ParameterClass params;
      ccsd::CcsdSolver solver(params, session.rank(), session.size());
      solver.run();
      return 0;
  }
  ```

For files that share the `CcsdSolver` private state, declare those methods as `friend` free functions in `intermediates.hpp` etc., **or** inline them as `static` member functions. Pick the friend-free-function style — it keeps the class definition small and lets each `.cpp` see only what it needs. The data members of `CcsdSolver` are exposed only through getters used inside the friend functions — keep this minimal to avoid leaking state.

- [ ] **Step 11.5: Replace the top-level `CMakeLists.txt`.**

```cmake
cmake_minimum_required(VERSION 3.21)

project(ccsd_hehp_cpp VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ProjectWarnings)

option(CCSD_ENABLE_ASAN     "AddressSanitizer"  OFF)
option(CCSD_ENABLE_UBSAN    "UBSanitizer"       OFF)
option(CCSD_ENABLE_TSAN     "ThreadSanitizer"   OFF)
option(CCSD_ENABLE_COVERAGE "Code coverage"     OFF)

function(ccsd_apply_flags TARGET)
    project_apply_warnings(${TARGET})
    if(CCSD_ENABLE_ASAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
        target_link_options   (${TARGET} PUBLIC -fsanitize=address)
    endif()
    if(CCSD_ENABLE_UBSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=undefined)
        target_link_options   (${TARGET} PUBLIC -fsanitize=undefined)
    endif()
    if(CCSD_ENABLE_TSAN)
        target_compile_options(${TARGET} PUBLIC -fsanitize=thread)
        target_link_options   (${TARGET} PUBLIC -fsanitize=thread)
    endif()
    if(CCSD_ENABLE_COVERAGE)
        target_compile_options(${TARGET} PUBLIC --coverage -O0 -g)
        target_link_options   (${TARGET} PUBLIC --coverage)
    endif()
endfunction()

find_package(MPI REQUIRED)

include(FetchContent)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(src)

# Top-level executable
add_executable(ccsd_code apps/ccsd_code.cpp)
target_link_libraries(ccsd_code PRIVATE ccsd_core MPI::MPI_CXX)
ccsd_apply_flags(ccsd_code)

# Copy config.json to build dir for runtime convenience
configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)

if(BUILD_TESTING)
    enable_testing()
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    add_subdirectory(tests)
endif()
```

- [ ] **Step 11.6: Create `src/CMakeLists.txt`.**

```cmake
add_library(ccsd_core
    driver.cpp
    intermediates.cpp
    amplitudes.cpp
    energy.cpp)

target_include_directories(ccsd_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(ccsd_core
    PUBLIC MPI::MPI_CXX nlohmann_json::nlohmann_json)

target_compile_features(ccsd_core PUBLIC cxx_std_23)

ccsd_apply_flags(ccsd_core)
```

- [ ] **Step 11.7: Replace `tests/CMakeLists.txt`.**

```cmake
add_executable(test_tensors    test_tensors.cpp)
add_executable(test_parameters test_parameters.cpp)

foreach(t test_tensors test_parameters)
    target_link_libraries(${t} PRIVATE ccsd_core Catch2::Catch2WithMain)
    ccsd_apply_flags(${t})
    catch_discover_tests(${t})
endforeach()
```

- [ ] **Step 11.8: Update test `#include` paths.**

In `tests/test_tensors.cpp`:
- `#include "../VectorsClass.h"` → `#include <ccsd/tensors.hpp>`.
- Drop the `using Vector2D = ccsd::Vector2D;` aliases that come from the header; the test already uses `ccsd::Vector2D`.

In `tests/test_parameters.cpp`:
- `#include "../ParameterClass.h"` → `#include <ccsd/parameters.hpp>`.
- If `ParameterClass` was renamed under namespace `ccsd::ParameterClass`, update test references accordingly.

- [ ] **Step 11.9: Update `Doxyfile` paths to point at `include/` and `src/` (light edit).**

Find `INPUT =` in `Doxyfile`. Set:
```
INPUT = include src apps docs
```
Anything else can stay.

- [ ] **Step 11.10: Build, test, regress.**

```bash
rm -rf build/
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
python3 tests/run_mpi_regression.py --executable build/debug/ccsd_code
```
Expected: tests green; regression last line `All MPI regression tests passed.`

- [ ] **Step 11.11: Coverage gate becomes blocking — verify it can hit 100%.**

```bash
cmake --preset coverage && cmake --build --preset coverage
ctest --preset coverage
python3 tests/run_mpi_regression.py --executable build/coverage/ccsd_code
gcovr --root . --filter 'src/' --filter 'include/' --txt --print-summary \
      --fail-under-line 100 --object-directory build/coverage
```
Expected: gcovr exits 0 and prints `Lines: 100%`.

If coverage is below 100%: investigate the uncovered lines. Two acceptable resolutions: (a) add a unit test that exercises the path, or (b) **delete the unreachable code** (typical for the old `"error, out of range"` branches that T07 was supposed to remove — verify they really are gone). Do **not** lower the gate.

- [ ] **Step 11.12: Update CI to enforce coverage.**

Edit `.github/workflows/ci.yml` — in the `coverage` job, change the final step from:
```yaml
        run: |
          gcovr --root . --filter 'src/' --filter 'include/' --txt \
                --print-summary --object-directory build/coverage || true
```
to:
```yaml
        run: |
          gcovr --root . --filter 'src/' --filter 'include/' --txt \
                --print-summary --fail-under-line 100 \
                --object-directory build/coverage
```

- [ ] **Step 11.13: Update `Makefile` paths.**

The `regression` and `coverage` targets reference `build/debug/ccsd_code` and `build/coverage/ccsd_code` — keep as-is. Verify by running `make build && make regression`. Expected: green.

- [ ] **Step 11.14: Commit (one large commit; keep diff readable via `git mv`).**

```bash
git add include/ src/ apps/ tests/ CMakeLists.txt .github/workflows/ci.yml Doxyfile
git commit -m "refactor: migrate to include/src/apps layout; enforce 100% line coverage"
```

---

## Task 12: T12 — Flip `-Werror` on; activate clang-tidy in CI and pre-commit

**Files:**
- Modify: `cmake/ProjectWarnings.cmake`
- Modify: `.clang-tidy`
- Modify: `.github/workflows/ci.yml`
- Modify: `.pre-commit-config.yaml`
- (Possibly) Modify: many `.cpp`/`.hpp` files to fix warnings the new gate surfaces.

- [ ] **Step 12.1: Add `-Werror` to `cmake/ProjectWarnings.cmake`.**

Change the GCC/Clang branch from:
```cmake
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            ...
            -Wformat=2 -Wimplicit-fallthrough)
```
to:
```cmake
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Werror -Wpedantic
            ...
            -Wformat=2 -Wimplicit-fallthrough)
```

And in the MSVC branch, change `/W4 /permissive-` to `/W4 /WX /permissive-`.

- [ ] **Step 12.2: Build and fix all surfaced warnings.**

Run: `cmake --build --preset debug 2>&1 | tee build_warnings.log`

For each warning the build now turns into an error:
- Fix at the source if it's a real issue (uninitialized, sign-conversion, shadow).
- If the warning is for code we can't touch (third-party header), narrowly suppress with `#pragma GCC diagnostic` blocks around just that include.

The known categories from the baseline build:
- `-Wsign-compare` in `get_spinints`-style loops — change `size_t p = 1; p < dim2+1` to `int p = 1; p < dim2 + 1`.
- `-Wconversion` / `-Wsign-conversion` — add explicit `static_cast` where the compiler is right.
- `-Wshadow` — rename inner `i`/`j`/`k`/`l` if they shadow outer variables (post-T08 they shouldn't).

After each batch of fixes, re-run the build and the bit-exact regression. **Every fix must keep the regression green.**

- [ ] **Step 12.3: Turn on `-Werror` in `.clang-tidy`.**

Change:
```yaml
WarningsAsErrors: ''
```
to:
```yaml
WarningsAsErrors: '*'
```

Run: `make tidy`. Fix any reported issues — they are likely modernization opportunities (`modernize-*`), readability fixes, or `cppcoreguidelines-*` items.

- [ ] **Step 12.4: Add a clang-tidy CI job.**

Append to `.github/workflows/ci.yml`:
```yaml
  tidy:
    name: clang-tidy
    runs-on: ubuntu-22.04
    needs: debug
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-13 libopenmpi-dev openmpi-bin \
                                  ninja-build cmake clang-tidy-14 python3
          sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
          sudo update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-14 100
      - name: Configure
        run: cmake --preset debug
      - name: Build (for compile_commands.json)
        run: cmake --build --preset debug
      - name: Run clang-tidy
        run: |
          clang-tidy -p build/debug --warnings-as-errors='*' \
            $(git ls-files 'src/*.cpp' 'apps/*.cpp' 'include/ccsd/*.hpp')
```

- [ ] **Step 12.5: Add clang-tidy to pre-commit.**

Edit `.pre-commit-config.yaml`. Append:
```yaml
  - repo: local
    hooks:
      - id: clang-tidy
        name: clang-tidy
        language: system
        entry: clang-tidy -p build/debug --warnings-as-errors=*
        types: [c++]
        pass_filenames: true
        stages: [pre-commit]
        require_serial: true
```

- [ ] **Step 12.6: Verify `make check` from a fresh clone simulation.**

```bash
rm -rf build/
make check
```
Expected: every step (format, tidy, test, regression) green; final line of regression `All MPI regression tests passed.`

- [ ] **Step 12.7: Commit.**

```bash
git add cmake/ProjectWarnings.cmake .clang-tidy .github/workflows/ci.yml \
        .pre-commit-config.yaml
# also any .cpp/.hpp files modified to clear warnings
git add -A
git commit -m "build: flip -Werror on; enforce clang-tidy in CI and pre-commit"
```

- [ ] **Step 12.8: Open the merge PR.**

```bash
git push origin pass-1
gh pr create --base main --head pass-1 --title "Pass 1: modernize CCSD-HeH+ codebase" \
  --body "$(cat <<'EOF'
## Summary
- Bit-exact regression gate (T01) → modern build with presets/warnings/sanitizers (T02)
  → CI (T03) → .claude + pre-commit (T04) → header hygiene (T05) → RAII MpiSession
  (T06) → tensor API + decoupled MPI (T07) → macro-loop removal (T08) →
  CcsdSolver replaces globals (T09) → nlohmann/json + name-keyed config (T10) →
  include/src/apps layout + 100% coverage (T11) → -Werror + clang-tidy enforced (T12).
- Energies preserved bit-for-bit through every commit on the branch.

## Test plan
- [ ] CI: debug job green
- [ ] CI: asan+ubsan job green
- [ ] CI: coverage job green with 100% line coverage on src/ and include/ccsd/
- [ ] CI: clang-tidy job green
- [ ] Local: `make check` green from a clean clone
EOF
)"
```

---

## Self-review notes

- Spec coverage cross-check (each spec section / item → task):
  - §1 invariant — T01 (gate), every task's verification step.
  - §2 non-goals — implied throughout; called out in `.claude/CLAUDE.md` (T04).
  - §3 method — T01–T12 sequence on `pass-1` branch.
  - §4 toolchain — T02 (CMake/Catch2/clang-format/clang-tidy/sanitizers), T03 (CI), T10 (nlohmann/json), T12 (-Werror).
  - §5 layout — T11.
  - §6 ticket backlog — T01–T12 1-to-1.
  - §7 build modes — T02.
  - §8 CI matrix — T03 (debug/asan/coverage), T11 (coverage gate flips on), T12 (clang-tidy job).
  - §9 test layering — T07 (test_tensors), T10 (test_parameters), Python regression throughout.
  - §10 numerical-determinism rules — enforced by bit-exact gate; called out in `.claude/CLAUDE.md`.
  - §11 risks — `MPI_Init` only in `MpiSession` ctor (T06 step 6.5); nlohmann/json parity verified by bit-exact regression (T10 step 10.9); GCC 13.1 missing C++23 stdlib bits — never used; coverage 100% achievable because dead branches deleted in T07.
  - §12 pre-edit discipline — T01 (gate first), T02 (alongside legacy), T04 (.claude in repo), T11 (layout migration is its own ticket), T12 (Werror last), T01.4 (gate-failure demo).
  - §13 pass-2 candidates — out of scope; not implemented.

- Type/symbol consistency check:
  - `ccsd::MpiSession` (T06) → used in T11 step 11.4's `apps/ccsd_code.cpp`.
  - `ccsd::Vector2D`/`Vector4D` (T07) → used in T08–T11.
  - `ccsd::CcsdSolver` (T09) → consumed in T11.
  - `ccsd::ParameterClass` (T10) → consumed in T11. Note: T10's class lives in the *global* namespace as `ParameterClass`. T11 step 11.4 references `ccsd::ParameterClass`. Resolution: in T11 step 11.2's "rename to namespace ccsd" implicit work, wrap `ParameterClass` in `namespace ccsd { ... }` when moving to `include/ccsd/parameters.hpp`. T11 step 11.8 already mentions updating tests for the namespace move.

- Placeholder scan: no `TBD`/`TODO`/"implement later" tokens. Steps that move large code blocks (T09 step 9.3, T11 step 11.4) reference exact source-of-truth functions, exact rewrites of identifier prefixes, and exact verification gates.

---

## Coverage tally

| Spec ticket | Plan task | Files touched | Tests added |
|---|---|---|---|
| T01 | Task 1  | `tests/run_mpi_regression.py` | regression now bit-exact |
| T02 | Task 2  | `cmake/ProjectWarnings.cmake`, `CMakePresets.json`, `.clang-format`, `.clang-tidy`, `Makefile`, `CMakeLists.txt`, `tests/test_smoke.cpp`, `.gitignore` | `test_smoke` (placeholder) |
| T03 | Task 3  | `.github/workflows/ci.yml` | CI runs the regression |
| T04 | Task 4  | `.claude/CLAUDE.md`, `.claude/agents/ccsd-reviewer.md`, `.gitignore`, `.pre-commit-config.yaml` | format/yaml hooks |
| T05 | Task 5  | `ParameterClass.h`, `VectorsClass.h`, `MpiClass.h` (audit), `ccsd_code.cpp`, `CMakeLists.txt`; deletes `ParameterClass_NoJson.h` | none |
| T06 | Task 6  | `MpiClass.h`, `ccsd_code.cpp` | none (covered by regression) |
| T07 | Task 7  | `VectorsClass.h`, `include/ccsd/mpi_tensor.hpp`, `ccsd_code.cpp`, `tests/test_tensors.cpp`, `CMakeLists.txt` | `test_tensors` (4 cases) |
| T08 | Task 8  | `ccsd_code.cpp` | none (covered by regression) |
| T09 | Task 9  | `ccsd_code.cpp` | none (covered by regression) |
| T10 | Task 10 | `CMakeLists.txt`, `ParameterClass.h`, `ccsd_code.cpp`, `config.json`, `tools/migrate_config.py`, `tests/test_parameters.cpp` | `test_parameters` |
| T11 | Task 11 | full layout migration; `Doxyfile`; `.github/workflows/ci.yml` | gates 100% line coverage |
| T12 | Task 12 | `cmake/ProjectWarnings.cmake`, `.clang-tidy`, `.github/workflows/ci.yml`, `.pre-commit-config.yaml`; bug-fix sweep | none new |
