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
