# Pass-2 Baseline Profile (gprof)

Source: `gprof build/release-gprof/ccsd_bench gmon.out` after
`./ccsd_bench --batch 5000 --warmup 50` (single rank).

The plan's recommended `--batch 200` produced a degenerate single-tick
profile (resolution ~10 ms vs. ~150 us per iter); batch was raised to
5000 so per-function self-time accumulates. Total sampled run: 0.57 s.

Top-10 hottest functions (from profile-baseline.txt):

| % time | self s | function |
|--------|--------|----------|
| 47.37  | 0.27   | `CcsdSolver::makeT2_d()` |
| 10.53  | 0.06   | `CcsdSolver::get_Wmbej()` |
|  8.77  | 0.05   | `CcsdSolver::makeT1_s()` |
|  5.26  | 0.03   | `CcsdSolver::get_Wabef()` |
|  5.26  | 0.03   | `CcsdSolver::get_Wmnij()` |
|  5.26  | 0.03   | `CcsdSolver::get_spinints()` |
|  3.51  | 0.02   | `CcsdSolver::get_Fae()` |
|  3.51  | 0.02   | `std::vector<double>::_M_fill_assign(...)` |
|  1.75  | 0.01   | `CcsdSolver::get_Fme()` |
|  1.75  | 0.01   | `CcsdSolver::get_Fmi()` |

The top three named CCSD kernels — `makeT2_d`, `get_Wmbej`, `makeT1_s` —
together account for ~67% of self-time and are the loops we'll target
for OpenMP in Wave 4. `get_Wabef` / `get_Wmnij` / `get_Fae` round out
the next tier (~14% combined). The `_M_fill_assign` row (~3.5%) is
zeroing of working buffers inside the same kernels and will largely go
away once those loops are parallelized over the outermost index.
