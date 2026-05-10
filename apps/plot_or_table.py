#!/usr/bin/env python3
"""Collate raw bench JSONs into the markdown report.

Per (preset, np, threads): keep the best-of-N median across repetitions.
Speedups computed against the row labelled "release" with np=4 threads=1
(the canonical baseline pinned in P05).
"""

from __future__ import annotations

import json
import sys
from collections import defaultdict
from pathlib import Path


def main() -> None:
    if len(sys.argv) != 3:
        print("usage: plot_or_table.py <raw-dir> <out-md>", file=sys.stderr)
        sys.exit(2)
    raw_dir = Path(sys.argv[1])
    out_md = Path(sys.argv[2])

    # group rows by (preset, np, threads); keep the best p50 across reps
    grouped: dict[tuple[str, int, int], list[dict]] = defaultdict(list)
    for p in sorted(raw_dir.glob("*.json")):
        d = json.load(open(p))
        key = (d["preset"], d["np"], d["threads_per_rank"])
        grouped[key].append(d)

    rows = []
    for key, entries in sorted(grouped.items()):
        best = min(entries, key=lambda e: e["per_iter_us"]["p50"])
        rows.append((key, best))

    # Baseline: release / np=4 / threads=1. Falls back to any release if absent.
    baseline_p50 = None
    for (preset, np, t), best in rows:
        if preset == "release" and np == 4 and t == 1:
            baseline_p50 = best["per_iter_us"]["p50"]
            break
    if baseline_p50 is None:
        for (preset, np, t), best in rows:
            if preset == "release":
                baseline_p50 = best["per_iter_us"]["p50"]
                break

    lines = [
        "# Pass-2 Benchmark Results",
        "",
        "Updated automatically by `benchmarks/plot_or_table.py`. Best of 3 medians per row.",
        "",
        "| Preset | np | threads | per-iter μs (p50) | speedup vs baseline |",
        "|--------|----|---------|-------------------|---------------------|",
    ]
    for (preset, np, t), best in rows:
        p50 = best["per_iter_us"]["p50"]
        speedup = (baseline_p50 / p50) if baseline_p50 else float("nan")
        lines.append(f"| {preset} | {np} | {t} | {p50:.0f} | {speedup:.2f}× |")

    out_md.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
