#!/usr/bin/env python3
"""Regression test runner for the CCSD MPI executable (two-tier).

Strict tier (default, --tolerance 0): asserts the two energy lines appear
verbatim in stdout. Tolerance tier (--tolerance T > 0): parses the printed
energies and asserts |actual - reference| < T.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

# Reference energies. EXPECTED_LINES is the canonical printed form (used by
# strict tier); REF_E_* are the parsed-float counterparts (used by tolerance
# tier). Keep these three in lockstep — when a reference value changes, all
# three constants update together.
REF_E_CORR = -0.008225832259
REF_E_TOTAL = -2.862598243
EXPECTED_LINES = (
    "  E(corr,CCSD) = -0.008225832259",
    "  E(CCSD) = -2.862598243",
)
DEFAULT_PROCESSES = (2, 4, 8)
DEFAULT_EXECUTABLE = "build/ccsd_code"

CORR_RE = re.compile(r"E\(corr,CCSD\)\s*=\s*([-\d.eE+]+)")
TOTAL_RE = re.compile(r"E\(CCSD\)\s*=\s*([-\d.eE+]+)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Two-tier CCSD MPI regression.")
    parser.add_argument("--executable", default=DEFAULT_EXECUTABLE)
    parser.add_argument("--mpirun", default=shutil.which("mpirun") or "mpirun")
    parser.add_argument("--np", nargs="+", type=int, default=list(DEFAULT_PROCESSES))
    parser.add_argument("--no-oversubscribe", action="store_true")
    parser.add_argument(
        "--tolerance", type=float, default=0.0,
        help="0 = bit-exact string match (default). >0 = abs-tolerance on parsed energies.",
    )
    args = parser.parse_args()
    if args.tolerance < 0.0:
        parser.error("--tolerance must be >= 0")
    return args


def _assert_strict(stdout: str, np: int) -> None:
    """Strict tier: every reference line must appear verbatim in stdout."""
    stdout_lines = stdout.splitlines()
    for expected in EXPECTED_LINES:
        if expected not in stdout_lines:
            sys.stderr.write(stdout)
            raise AssertionError(
                f"[np={np}] expected line not found verbatim:\n"
                f"  expected: {expected!r}"
            )
    print(f"[np={np}] PASS  (bit-exact)")


def _assert_within_tolerance(stdout: str, np: int, tol: float) -> None:
    """Tolerance tier: parse energies and assert |actual - reference| < tol."""
    m_corr = CORR_RE.search(stdout)
    m_total = TOTAL_RE.search(stdout)
    if not (m_corr and m_total):
        sys.stderr.write(stdout)
        raise AssertionError(f"[np={np}] could not parse energies from output")
    e_corr = float(m_corr.group(1))
    e_total = float(m_total.group(1))
    if abs(e_corr - REF_E_CORR) >= tol:
        raise AssertionError(
            f"[np={np}] E_corr off by {abs(e_corr - REF_E_CORR):.3e} "
            f"(tolerance {tol:.3e})"
        )
    if abs(e_total - REF_E_TOTAL) >= tol:
        raise AssertionError(
            f"[np={np}] E_total off by {abs(e_total - REF_E_TOTAL):.3e} "
            f"(tolerance {tol:.3e})"
        )
    print(f"[np={np}] PASS  (tolerance {tol:.0e})")


def run_case(args: argparse.Namespace, np: int) -> None:
    exe = Path(args.executable)
    if not exe.exists():
        raise FileNotFoundError(f"Executable '{exe}' not found. Build first.")

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

    if args.tolerance == 0.0:
        _assert_strict(result.stdout, np)
    else:
        _assert_within_tolerance(result.stdout, np, args.tolerance)


def main() -> None:
    args = parse_args()
    for np in args.np:
        run_case(args, np)
    print("All MPI regression tests passed.")


if __name__ == "__main__":
    main()
