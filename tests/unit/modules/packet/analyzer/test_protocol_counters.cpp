#include "catch.hpp"

#include "packet/analyzer/statistics/protocol/counters.hpp"

using namespace openperf::packet::analyzer::statistics;

TEST_CASE("protocol counters", "[packet_analyzer]")
{
    using test_vec = std::vector<uint32_t>;

    auto result = std::
        tuple<counter, protocol::ethernet, protocol::ip, protocol::protocol>{};

    const auto test_values = test_vec{0x111, 0x211, 0x141, 0x241};

    protocol::update(result, test_values.data(), test_values.size());

    SECTION("ethernet counters, ")
    {
        auto& eth_counters = get_counter<protocol::ethernet>(result);
        REQUIRE(eth_counters[0x1] == test_values.size());
    }

    SECTION("ip counters, ")
    {
        auto& ip_counters = get_counter<protocol::ip>(result);
        REQUIRE(ip_counters[0x1] == 2);
        REQUIRE(ip_counters[0x4] == 2);
    }

    SECTION("protocol counters, ")
    {
        auto& protocol_counters = get_counter<protocol::protocol>(result);
        REQUIRE(protocol_counters[0x1] == 2);
        REQUIRE(protocol_counters[0x2] == 2);
    }
}
