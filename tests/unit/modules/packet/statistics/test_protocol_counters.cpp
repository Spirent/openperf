#include "catch.hpp"

#include "packet/statistics/tuple_utils.hpp"
#include "packet/statistics/protocol/packet_type_counters.hpp"

using namespace openperf::packet::statistics;

TEST_CASE("protocol counters", "[packet_protocol]")
{
    using test_vec =
        std::vector<openperf::packetio::packet::packet_type::flags>;

    auto result =
        std::tuple<protocol::ethernet, protocol::ip, protocol::transport>{};

    const auto test_values = test_vec{0x111, 0x211, 0x141, 0x241};

    update(result, test_values.data(), test_values.size());

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

    SECTION("transport counters, ")
    {
        auto& transport_counters = get_counter<protocol::transport>(result);
        REQUIRE(transport_counters[0x1] == 2);
        REQUIRE(transport_counters[0x2] == 2);
    }
}
