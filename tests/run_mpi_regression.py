#!/usr/bin/env python3
"""
Simple regression test runner for the CCSD MPI executable.

The script launches the compiled binary with several MPI process counts and
checks that the reported CCSD energies match the expected reference values
within a small tolerance. Use this to quickly make sure code changes did not
alter the numerical results.
"""

from __future__ import annotations

import argparse
import math
import re
import shutil
import subprocess
import sys
from pathlib import Path

EXPECTED_CORR = -0.008225832259
EXPECTED_TOTAL = -2.862598243
DEFAULT_PROCESSES = (2, 4, 8)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run CCSD regression tests across multiple MPI sizes."
    )
    parser.add_argument(
        "--executable",
        default="./ccsd_code",
        help="Path to the CCSD executable (default: ./ccsd_code)",
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
        "--tol",
        type=float,
        default=1e-9,
        help="Absolute tolerance for comparing energies (default: 1e-9)",
    )
    parser.add_argument(
        "--no-oversubscribe",
        action="store_true",
        help="Do not append --oversubscribe to mpirun invocations.",
    )
    return parser.parse_args()


def extract_energy(pattern: re.Pattern[str], text: str, label: str) -> float:
    match = pattern.search(text)
    if not match:
        raise ValueError(f"Could not find {label} in output:\n{text}")
    return float(match.group(1))


def run_case(args: argparse.Namespace, np: int) -> None:
    exe = Path(args.executable)
    if not exe.exists():
        raise FileNotFoundError(f"Executable '{exe}' not found. Build ccsd_code first.")

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

    combined_output = "\n".join([result.stdout.strip(), result.stderr.strip()]).strip()
    corr = extract_energy(
        re.compile(r"E\(corr,CCSD\)\s*=\s*([-\d.eE+]+)"), combined_output, "correlation energy"
    )
    total = extract_energy(
        re.compile(r"E\(CCSD\)\s*=\s*([-\d.eE+]+)"), combined_output, "total energy"
    )

    if not math.isclose(corr, EXPECTED_CORR, rel_tol=0.0, abs_tol=args.tol):
        raise AssertionError(
            f"[np={np}] correlation energy mismatch: got {corr}, expected {EXPECTED_CORR}"
        )
    if not math.isclose(total, EXPECTED_TOTAL, rel_tol=0.0, abs_tol=args.tol):
        raise AssertionError(
            f"[np={np}] total energy mismatch: got {total}, expected {EXPECTED_TOTAL}"
        )
    print(f"[np={np}] PASS  (Ecorr={corr:.12f}, Etotal={total:.12f})")


def main() -> None:
    args = parse_args()
    for np in args.np:
        run_case(args, np)
    print("All MPI regression tests passed.")


if __name__ == "__main__":
    main()
