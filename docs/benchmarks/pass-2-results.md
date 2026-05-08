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
| release-omp | 2 | 1 | 270 | 1.07× |
| release-omp | 2 | 4 | 1030 | 0.28× |
| release-omp | 4 | 1 | 347 | 0.84× |
| release-omp | 4 | 4 | 357 | 0.81× |
