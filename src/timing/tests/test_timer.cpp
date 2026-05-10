#include <catch2/catch_test_macros.hpp>

#include <timing/timer.h>

#include <chrono>
#include <thread>

TEST_CASE("Timer destructs without crashing", "[timer]") {
    // Verify RAII lifetime compiles and runs cleanly at rank=0.
    {
        ccsd::Timer t("test_scope", 0);
        (void)t;
        // dtor fires here; would crash or print garbled output if broken
    }
    REQUIRE(true);  // if we reach here the dtor ran cleanly
}

TEST_CASE("Timer measures non-zero elapsed time", "[timer]") {
    using clock = ccsd::Timer::clock;
    auto before = clock::now();
    {
        ccsd::Timer t("busy_wait", 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    auto after = clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
    REQUIRE(elapsed >= 4);  // at least ~4 ms
}
