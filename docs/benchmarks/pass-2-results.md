# Pass-2 Benchmark Results

Updated automatically by `benchmarks/plot_or_table.py`. Best of 3 medians per row.

| Preset | np | threads | per-iter μs (p50) | speedup vs baseline |
|--------|----|---------|-------------------|---------------------|
| release | 2 | 1 | 247 | 1.17× |
| release | 4 | 1 | 290 | 1.00× |
| release-fast | 4 | 1 | 281 | 1.03× |
| release-fast-pgo | 4 | 1 | 281 | 1.03× |
| release-mdspan | 4 | 1 | 301 | 0.96× |
| release-mdspan-rowmajor | 4 | 1 | 299 | 0.97× |
| release-omp | 1 | 1 | 162 | 1.79× |
| release-omp | 1 | 2 | 228 | 1.27× |
| release-omp | 1 | 4 | 973 | 0.30× |
| release-omp | 2 | 1 | 281 | 1.03× |
| release-omp | 2 | 2 | 355 | 0.82× |
| release-omp | 2 | 4 | 1041 | 0.28× |
| release-omp | 4 | 1 | 337 | 0.86× |
| release-omp | 4 | 2 | 343 | 0.85× |
| release-omp | 4 | 4 | 354 | 0.82× |
| release-omp | 8 | 1 | 350 | 0.83× |
| release-omp | 8 | 2 | 363 | 0.80× |
| release-omp | 8 | 4 | 389 | 0.75× |
| release-omp-pinned | 4 | 4 | 361 | 0.80× |

## Summary

| Wave | Knob                          | Best p50 (μs) | Speedup vs release/np=4/t=1 baseline (290 μs) |
|------|-------------------------------|----------------|------------------------------------------------|
| 1    | release (baseline)            | 290            | 1.00×                                          |
| 2    | -O3 -march=native -flto       | 281            | 1.03×                                          |
| 2    | release-fast-pgo              | 281            | 1.03×                                          |
| 3    | mdspan view (col-major)       | 301            | 0.96×                                          |
| 3    | row-major layout              | 299            | 0.97×                                          |
| 4    | OpenMP threads=1, np=1        | 162            | 1.79×                                          |
| 4    | OpenMP×MPI sweet spot found   | 162 (np=1,t=1) | 1.79×                                          |
| 5    | NUMA pinning (np=4, t=4)      | 361            | 0.80×                                          |
| 5    | Collective-ops audit          | n/a            | (no code change; existing pattern is optimal)  |
| 6    | Multi-node                    | n/a            | (cluster unavailable; node-only run)           |

## Cluster validation

No cluster available at the time of writing; numbers are node-only on a
dual-socket Intel Xeon E5-2650 v3 @ 2.30 GHz (40 logical CPUs, 2 NUMA
nodes). Multi-node validation is deferred to pass-3.

## Final assessment

For HeH⁺ in STO-3G — tensor sizes ≤ 256 doubles per `Vector4D` — the
problem is too small to benefit from cluster-scale parallelism on this
hardware. The recorded numbers point to **(np=1, threads=1) at 162 μs /
1.79× over the np=4 release baseline** as the optimum on this node.
Beyond that:

- `-O3 -march=native -flto` adds ~3% over `-O2`. Modest and within
  measurement noise.
- PGO produced no measurable speedup over `release-fast`. The hot path
  is already well-optimized by the static heuristics.
- mdspan-backed views and row-major layout did not help. The hand-rolled
  `Vector2D/4D` access pattern was already cache-friendly for this size.
- OpenMP within a rank is pure overhead at this problem size — fork/join
  for sub-millisecond iterations dominates the gain. The tolerance gate
  (1e-9) verified that OMP did not break numerics.
- NUMA pinning at the (np=4, t=4) configuration did not recover the
  oversubscription overhead.
- Collective-op audit confirmed the existing scatter→gather→broadcast
  pattern is already optimal for this workload.

**Pass-2 takeaway:** the existing CCSD-HeH⁺ kernel is already near-optimal
for its problem size on this hardware. The pass exposed where parallelism
helps (it doesn't, here) and where it costs (everywhere). Pass-3
candidates (BLAS, GPU, larger N) are appropriate for any future speedup
work.
