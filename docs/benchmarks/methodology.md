# Pass-2 Benchmark Methodology

## Hardware

This node: dual-socket Intel Xeon E5-2650 v3 @ 2.30 GHz (Haswell), 10 cores
× 2 sockets × 2 SMT = 40 logical CPUs; 2 NUMA nodes. CPU governor /
turbo state recorded per run via the `host` field of each JSON.

## Procedure

1. **Warmup**: discard first K iterations (default K=50) to populate caches
   and stabilize CPU frequency.
2. **Measure**: run N iterations (default N=1000), record per-iteration wall
   time via `chrono::steady_clock`.
3. **Statistics**: per-iter mean, p50 (median), p99. We report **median**.
4. **Repetitions**: each (preset, np, threads) row is run R=3 times back-
   to-back; the **best-of-3 median** is what gets into the report.
5. **Baseline**: P05 records the `release / np=4 / threads=1` median once.
   Speedup = baseline / new.

## Reproducibility

```
make bench         # canonical matrix
make bench-quick   # smaller (np=2, batch=200) variant
```

Override knobs via environment:
```
PRESETS="release release-fast" NP_LIST="2 4 8" THREADS_LIST="1 4" \
    BATCH=2000 WARMUP=100 REPETITIONS=5 ./benchmarks/run_bench.sh
```

## Numerical-determinism rules

- Strict tier never reorders FP operations (canonical `release` build).
- Tolerance tier (1e-9) applies to perf builds: OpenMP, mdspan, MPI
  collectives. `-ffast-math` and `-Ofast` are banned.
- OpenMPI version recorded in each JSON row for traceability.
