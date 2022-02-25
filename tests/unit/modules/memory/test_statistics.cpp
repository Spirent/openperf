#include <chrono>

#include "catch.hpp"

#include "memory/generator/statistics.hpp"

using test_clock = std::chrono::system_clock;
using io_stats = openperf::memory::generator::io_stats<test_clock>;
using thread_stats = openperf::memory::generator::thread_stats<test_clock>;

static io_stats get_io_stats(int x)
{
    auto stat = io_stats();
    stat.bytes_actual = x;
    stat.bytes_target = x;
    stat.latency = test_clock::duration{x};
    stat.ops_actual = x;
    stat.ops_target = x;

    return (stat);
}

static thread_stats get_thread_stats(int x)
{
    auto ts = thread_stats(test_clock::now());
    ts.read = get_io_stats(x);
    ts.write = get_io_stats(x);

    return (ts);
}

TEST_CASE("memory statistics", "[memory]")
{

    SECTION("check statistics")
    {
        auto start = test_clock::now();

        auto stats = std::vector<thread_stats>{};
        stats.emplace_back(get_thread_stats(1));
        stats.emplace_back(get_thread_stats(1));
        stats.emplace_back(get_thread_stats(2));

        auto stop = test_clock::now();

        auto sum = sum_stats(stats);

        REQUIRE(sum.first().has_value());
        REQUIRE(start <= sum.first().value());

        REQUIRE(sum.last().has_value());
        REQUIRE(sum.last().value() < stop);

        REQUIRE(sum.read.bytes_actual == 4);
        REQUIRE(sum.read.bytes_target == 4);
        REQUIRE(sum.read.latency == test_clock::duration{4});
        REQUIRE(sum.read.ops_actual == 4);
        REQUIRE(sum.read.ops_target == 4);

        REQUIRE(sum.write.bytes_actual == 4);
        REQUIRE(sum.write.bytes_target == 4);
        REQUIRE(sum.write.latency == test_clock::duration{4});
        REQUIRE(sum.write.ops_actual == 4);
        REQUIRE(sum.write.ops_target == 4);
    }
}
