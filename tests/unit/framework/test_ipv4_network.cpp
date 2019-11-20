#include "catch.hpp"

#include "net/ipv4_network.h"

using ipv4_address = openperf::net::ipv4_address;
using ipv4_network = openperf::net::ipv4_network;

TEST_CASE("ipv4_network functionality checks", "[ipv4_network]")
{
    SECTION("constructor functionality checks") {
        ipv4_network test(ipv4_address{169,254,0,1}, 16);

        REQUIRE(test.address() == ipv4_address{169,254,0,0});
        REQUIRE(test.prefix_length() == 16);
    }

    SECTION("comparison operator checks") {
        REQUIRE(ipv4_network(ipv4_address("198.51.100.10"), 24)
                == ipv4_network(ipv4_address("198.51.100.1"), 24));
        REQUIRE(ipv4_network(ipv4_address{198,51,100,10}, 24)
                == ipv4_network(ipv4_address("198.51.100.1"), 24));
    }
}
