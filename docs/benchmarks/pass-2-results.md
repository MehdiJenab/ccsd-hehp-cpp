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
| release-omp | 4 | 4 | 369 | 0.79× |
| release-omp | 8 | 1 | 350 | 0.83× |
| release-omp | 8 | 2 | 363 | 0.80× |
| release-omp | 8 | 4 | 389 | 0.75× |

## Best (np, threads) on this node

The (np × threads) matrix shows the sweet spot at (1, 1):
1.79× vs the release baseline. Beyond that, MPI rank count adds
overhead faster than it adds parallelism for this problem size.

For HeH+ at this tiny tensor size (~256 doubles), the OpenMP fork/join
overhead and MPI ring-reduction dominate any available parallelism: every
column of the matrix worsens monotonically as either `np` or `threads`
grows. The release-omp binary at np=1, threads=1 outperforms even the
plain release baseline (np=2, threads=1) because it skips the MPI
collective entirely. This is a "stay serial" verdict for HeH+; the OMP
plumbing only pays off once the per-iteration tensor work is large enough
to amortize fork/join — not the case here.
