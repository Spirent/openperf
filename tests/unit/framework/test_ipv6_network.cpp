#include "catch.hpp"

#include "net/ipv6_network.hpp"

using ipv6_address = openperf::net::ipv6_address;
using ipv6_network = openperf::net::ipv6_network;

TEST_CASE("ipv6_network functionality checks", "[ipv6_network]")
{
    SECTION("constructor functionality checks")
    {
        ipv6_network test(ipv6_address("2001:1:1::1"), 16);

        REQUIRE(test.address() == ipv6_address("2001::"));
        REQUIRE(test.prefix_length() == 16);
    }

    SECTION("comparison operator checks")
    {
        REQUIRE(ipv6_network(ipv6_address("2001:1:1::1001"), 124)
                == ipv6_network(ipv6_address("2001:1:1::1002"), 124));
        REQUIRE(ipv6_network(ipv6_address("2001:1:1::1001"), 124)
                != ipv6_network(ipv6_address("2001:1:1::1102"), 124));
    }
}
