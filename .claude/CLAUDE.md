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
