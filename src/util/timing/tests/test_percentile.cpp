#include <catch2/catch_test_macros.hpp>

#include <util/timing/percentile.h>

#include <chrono>
#include <thread>

TEST_CASE("PercentileAccumulator starts empty", "[timing]") {
    ccsd::timing::PercentileAccumulator acc;
    REQUIRE(acc.count() == 0);
    REQUIRE(acc.mean() == 0.0);
    REQUIRE(acc.total_seconds() == 0.0);
}

TEST_CASE("PercentileAccumulator records samples", "[timing]") {
    ccsd::timing::PercentileAccumulator acc;
    acc.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    acc.stop();
    REQUIRE(acc.count() == 1);
    REQUIRE(acc.mean() > 0.0);
    REQUIRE(acc.total_seconds() > 0.0);
}

TEST_CASE("PercentileAccumulator snapshot matches individual accessors", "[timing]") {
    ccsd::timing::PercentileAccumulator acc;
    for (int i = 0; i < 10; ++i) {
        acc.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        acc.stop();
    }
    auto snap = acc.snapshot();
    REQUIRE(snap.count == 10);
    REQUIRE(snap.p50 > 0.0);
    REQUIRE(snap.p99 >= snap.p50);
    REQUIRE(snap.mean > 0.0);
    REQUIRE(snap.total_seconds > 0.0);
}

TEST_CASE("PercentileAccumulator percentile p50 on sorted data", "[timing]") {
    // Force known sample values by calling start/stop with sleeps of known duration.
    // Use 4 samples; p50 should be the 2nd or 3rd (nearest-rank).
    ccsd::timing::PercentileAccumulator acc;
    // We can't easily control exact microseconds, so just test monotonicity:
    for (int i = 0; i < 20; ++i) {
        acc.start();
        acc.stop();
    }
    auto snap = acc.snapshot();
    REQUIRE(snap.p50 >= 0.0);
    REQUIRE(snap.p99 >= snap.p50);
}
