#include <ccsd/ccsd_solver.hpp>
#include <ccsd/timing.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace {

struct Args {
    int batch = 100;
    int warmup = 10;
    std::string report;
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            a.batch = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            a.warmup = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            a.report = argv[++i];
        }
    }
    return a;
}

}  // namespace

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);
    ccsd::MpiSession session(&argc, &argv);

    // Warmup: K solves, discarded.
    for (int i = 0; i < args.warmup; ++i) {
        CcsdSolver solver;
        solver.mpi.size = session.size();
        solver.mpi.rank = session.rank();
        solver.run();
    }

    ccsd::timing::PercentileAccumulator acc;
    for (int i = 0; i < args.batch; ++i) {
        CcsdSolver solver;
        solver.mpi.size = session.size();
        solver.mpi.rank = session.rank();
        acc.start();
        solver.run();
        acc.stop();
    }

    if (session.rank() == 0) {
        std::printf("ccsd_bench: np=%d batch=%d warmup=%d\n",
                    session.size(), args.batch, args.warmup);
        std::printf("  per-iter mean=%.0f us  p50=%.0f us  p99=%.0f us  total=%.3f s\n",
                    acc.mean(), acc.percentile(0.50), acc.percentile(0.99),
                    acc.total_seconds());

        if (!args.report.empty()) {
            std::ofstream out(args.report);
            out << "{\n";
            out << "  \"np\": " << session.size() << ",\n";
            out << "  \"batch\": " << args.batch << ",\n";
            out << "  \"warmup\": " << args.warmup << ",\n";
            out << "  \"wall_seconds\": " << acc.total_seconds() << ",\n";
            out << "  \"per_iter_us\": {\"mean\": " << acc.mean()
                << ", \"p50\": " << acc.percentile(0.50)
                << ", \"p99\": " << acc.percentile(0.99) << "}\n";
            out << "}\n";
        }
    }
    return 0;
}
