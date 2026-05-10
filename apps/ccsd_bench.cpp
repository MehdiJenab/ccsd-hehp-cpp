#include <solver/ccsd_solver.h>
#include <timing/percentile.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace {

struct Args {
    int         batch  = 100;
    int         warmup = 10;
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

void attach_session(CcsdSolver& solver, const ccsd::MpiSession& session) {
    solver.attach(session);
}

void run_warmup(const ccsd::MpiSession& session, int k) {
    for (int i = 0; i < k; ++i) {
        CcsdSolver solver;
        attach_session(solver, session);
        solver.run();
    }
}

void run_timed(const ccsd::MpiSession& session, int n,
               ccsd::timing::PercentileAccumulator& acc) {
    for (int i = 0; i < n; ++i) {
        CcsdSolver solver;
        attach_session(solver, session);
        acc.start();
        solver.run();
        acc.stop();
    }
}

void print_human_report(int np, const Args& args,
                        const ccsd::timing::PercentileAccumulator::Snapshot& snap) {
    std::printf("ccsd_bench: np=%d batch=%d warmup=%d\n", np, args.batch, args.warmup);
    std::printf("  per-iter mean=%.0f us  p50=%.0f us  p99=%.0f us  total=%.3f s\n",
                snap.mean, snap.p50, snap.p99, snap.total_seconds);
}

void write_json_report(const std::string& path, int np, const Args& args,
                       const ccsd::timing::PercentileAccumulator::Snapshot& snap) {
    std::ofstream out(path);
    out << "{\n";
    out << "  \"np\": " << np << ",\n";
    out << "  \"batch\": " << args.batch << ",\n";
    out << "  \"warmup\": " << args.warmup << ",\n";
    out << "  \"wall_seconds\": " << snap.total_seconds << ",\n";
    out << "  \"per_iter_us\": {\"mean\": " << snap.mean
        << ", \"p50\": " << snap.p50
        << ", \"p99\": " << snap.p99 << "}\n";
    out << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
    Args              args = parse_args(argc, argv);
    ccsd::MpiSession  session(&argc, &argv);

    run_warmup(session, args.warmup);

    ccsd::timing::PercentileAccumulator acc;
    run_timed(session, args.batch, acc);

    if (session.rank() == 0) {
        auto snap = acc.snapshot();
        print_human_report(session.size(), args, snap);
        if (!args.report.empty()) {
            write_json_report(args.report, session.size(), args, snap);
        }
    }
    return 0;
}
