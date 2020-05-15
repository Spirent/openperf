
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
    stat.time = std::chrono::nanoseconds(random());

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
        REQUIRE(st.time == st1.time + st2.time);

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

        REQUIRE(st.bytes == st1.bytes + st2.bytes);
        REQUIRE(st.bytes_target == st1.bytes_target + st2.bytes_target);
        REQUIRE(st.operations == st1.operations + st2.operations);
        REQUIRE(st.operations_target
                == st1.operations_target + st2.operations_target);
        REQUIRE(st.time == st1.time + st2.time);

        REQUIRE(st.latency_min.has_value());
        REQUIRE(st.latency_min.value() == st2.latency_min.value());
        REQUIRE(st.latency_max.has_value());
        REQUIRE(st.latency_max.value() == st2.latency_max.value());
    }
}