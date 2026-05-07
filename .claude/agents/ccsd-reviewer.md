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
