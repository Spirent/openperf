
#include <random>
#include "catch.hpp"
#include "memory/memory_stat.hpp"

using openperf::memory::internal::task_memory_stat;

auto random_memory_stat()
{
    static auto rnd = std::mt19937_64{};
    return task_memory_stat{
        .operations = rnd(),
        .operations_target = rnd(),
        .bytes = rnd(),
        .bytes_target = rnd(),
        .run_time = std::chrono::nanoseconds(rnd()),
        .sleep_time = std::chrono::nanoseconds(rnd()),
        .latency_min = std::chrono::nanoseconds(rnd()),
        .latency_max = std::chrono::nanoseconds(rnd()),
        .timestamp = task_memory_stat::timestamp_t(
            std::chrono::system_clock::now().time_since_epoch()),
        .errors = rnd(),
    };
}

TEST_CASE("memory stat addition", "[memory]")
{

    SECTION("two random")
    {
        auto st1 = random_memory_stat();
        auto st2 = random_memory_stat();
        auto st = st1 + st2;

        REQUIRE(st.bytes == st1.bytes + st2.bytes);
        REQUIRE(st.bytes_target == st1.bytes_target + st2.bytes_target);
        REQUIRE(st.operations == st1.operations + st2.operations);
        REQUIRE(st.operations_target
                == st1.operations_target + st2.operations_target);
        REQUIRE(st.run_time == st1.run_time + st2.run_time);
        REQUIRE(st.sleep_time == st1.sleep_time + st2.sleep_time);
        REQUIRE(st.errors == st1.errors + st2.errors);
        REQUIRE(st.timestamp == st2.timestamp);

        REQUIRE(st.latency_min.has_value());
        REQUIRE(st.latency_min.value()
                == std::min(st1.latency_min.value(), st2.latency_min.value()));
        REQUIRE(st.latency_max.has_value());
        REQUIRE(st.latency_max.value()
                == std::max(st1.latency_max.value(), st2.latency_max.value()));
    }

    SECTION("random + empty")
    {
        auto st1 = task_memory_stat{
            .timestamp = task_memory_stat::timestamp_t(
                std::chrono::system_clock::now().time_since_epoch())};
        auto st2 = random_memory_stat();
        auto st = st1 + st2;

        REQUIRE(st.bytes == st2.bytes);
        REQUIRE(st.bytes_target == st2.bytes_target);
        REQUIRE(st.operations == st2.operations);
        REQUIRE(st.operations_target == st2.operations_target);
        REQUIRE(st.run_time == st2.run_time);
        REQUIRE(st.sleep_time == st2.sleep_time);
        REQUIRE(st.errors == st2.errors);
        REQUIRE(st.timestamp == st2.timestamp);

        REQUIRE(st.latency_min.has_value());
        REQUIRE(st.latency_min.value() == st2.latency_min.value());
        REQUIRE(st.latency_max.has_value());
        REQUIRE(st.latency_max.value() == st2.latency_max.value());
    }
}
