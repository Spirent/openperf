#include "catch.hpp"

#include "packet/type/ipv4_address.hpp"

using namespace libpacket::type;

TEST_CASE("ipv4 address", "[libpacket]")
{
    SECTION("constructor functionality")
    {
        /* valid */
        ipv4_address ref(0x0A000001); /* 10.0.0.1 */
        REQUIRE(ipv4_address("10.0.0.1") == ref);
        REQUIRE(ipv4_address{10, 0, 0, 1} == ref);

        std::vector<uint8_t> addr{10, 0, 0, 1};
        REQUIRE(ipv4_address(addr.data()) == ref);

        /* invalid */
        REQUIRE_THROWS(ipv4_address("203.0.113"));
        REQUIRE_THROWS(ipv4_address("203.0.113.0.1"));
        REQUIRE_THROWS(ipv4_address{224, 0, 0, 0, 1});
    }

    SECTION("access by index")
    {
        ipv4_address test{198, 51, 100, 10};

        REQUIRE(test[0] == 198);
        REQUIRE(test[1] == 51);
        REQUIRE(test[2] == 100);
        REQUIRE(test[3] == 10);
        // REQUIRE_THROWS(test[4]);  /* undefined behavior */
        REQUIRE_THROWS(test.at(4));
    }

    SECTION("loopback recognition check")
    {
        REQUIRE(is_loopback(ipv4_address{127, 0, 0, 1}));
        REQUIRE(is_loopback(ipv4_address("127.0.0.10")));
        REQUIRE(!is_loopback(ipv4_address{192, 0, 2, 1}));
    }

    SECTION("multicast recognition check")
    {
        REQUIRE(is_multicast(ipv4_address{231, 1, 2, 3}));
        REQUIRE(is_multicast(ipv4_address("224.0.0.1")));
        REQUIRE(!is_multicast(ipv4_address{192, 0, 2, 1}));
    }

    SECTION("check comparison operators")
    {
        ipv4_address a{198, 0, 2, 1};
        ipv4_address b{198, 51, 100, 12};
        ipv4_address c("198.51.100.12");

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b <= c);
        REQUIRE(b > a);
        REQUIRE(b >= a);
        REQUIRE(b >= c);
        REQUIRE(b == c);
        REQUIRE(a != b);
    }

    SECTION("check string conversion")
    {
        REQUIRE(to_string(ipv4_address{203, 0, 113, 1}) == "203.0.113.1");
        REQUIRE(to_string(ipv4_address("203.0.113.2")) == "203.0.113.2");
        REQUIRE(to_string(ipv4_address(0xcb007103)) == "203.0.113.3");
    }
}
