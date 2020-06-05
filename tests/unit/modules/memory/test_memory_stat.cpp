
#include "catch.hpp"
#include "memory/memory_stat.hpp"

using memory_stat = openperf::memory::internal::memory_stat;

memory_stat random_memory_stat()
{
    auto stat = memory_stat{};
    stat.bytes = random();
    stat.bytes_target = random();
    stat.operations = random();
    stat.operations_target = random();
    stat.latency_max = std::chrono::nanoseconds(random());
    stat.latency_min = std::chrono::nanoseconds(random());
    stat.run_time = std::chrono::nanoseconds(random());
    stat.sleep_time = std::chrono::nanoseconds(random());
    stat.errors = random();

    return stat;
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
        auto st1 = memory_stat();
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