#include <array>

#include "catch.hpp"

#include "packet/type/ipv6_address.hpp"

using namespace libpacket::type;

TEST_CASE("ipv6 address", "[libpacket]")
{
    SECTION("constructor functionality checks")
    {
        // default constructor is all zero
        REQUIRE(ipv6_address().octets
                == std::array<uint8_t, 16>{
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

        /* valid */
        auto ref = ipv6_address("2001::1");
        REQUIRE(ipv6_address("2001::1") == ref);
        REQUIRE(
            ipv6_address{0x20, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
            == ref);

        std::vector<uint8_t> addr{
            0x20, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        REQUIRE(ipv6_address(addr.data()) == ref);

        /* invalid */
        REQUIRE_THROWS(ipv6_address("2001:1"));
        REQUIRE_THROWS(ipv6_address("1.0.0.1"));
    }

    SECTION("access by index")
    {
        ipv6_address test{0x10,
                          0x11,
                          0x12,
                          0x13,
                          0x14,
                          0x15,
                          0x16,
                          0x17,
                          0x18,
                          0x19,
                          0x1a,
                          0x1b,
                          0x1c,
                          0x1d,
                          0x1e,
                          0x1f};

        REQUIRE(test[0] == 0x10);
        REQUIRE(test[1] == 0x11);
        REQUIRE(test[2] == 0x12);
        REQUIRE(test[3] == 0x13);
        REQUIRE(test[4] == 0x14);
        REQUIRE(test[5] == 0x15);
        REQUIRE(test[6] == 0x16);
        REQUIRE(test[7] == 0x17);
        REQUIRE(test[8] == 0x18);
        REQUIRE(test[9] == 0x19);
        REQUIRE(test[10] == 0x1a);
        REQUIRE(test[11] == 0x1b);
        REQUIRE(test[12] == 0x1c);
        REQUIRE(test[13] == 0x1d);
        REQUIRE(test[14] == 0x1e);
        REQUIRE(test[15] == 0x1f);
        REQUIRE_THROWS(test.at(16));
    }

    SECTION("get u16")
    {
        auto addr = ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f");
        REQUIRE(addr.load<uint16_t>(0) == 0x1011);
        REQUIRE(addr.load<uint16_t>(1) == 0x1213);
        REQUIRE(addr.load<uint16_t>(2) == 0x1415);
        REQUIRE(addr.load<uint16_t>(3) == 0x1617);
        REQUIRE(addr.load<uint16_t>(4) == 0x1819);
        REQUIRE(addr.load<uint16_t>(5) == 0x1a1b);
        REQUIRE(addr.load<uint16_t>(6) == 0x1c1d);
        REQUIRE(addr.load<uint16_t>(7) == 0x1e1f);
        REQUIRE_THROWS(addr.load<uint16_t>(8));
    }

    SECTION("get u32")
    {
        auto addr = ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f");
        REQUIRE(addr.load<uint32_t>(0) == 0x10111213);
        REQUIRE(addr.load<uint32_t>(1) == 0x14151617);
        REQUIRE(addr.load<uint32_t>(2) == 0x18191a1b);
        REQUIRE(addr.load<uint32_t>(3) == 0x1c1d1e1f);
        REQUIRE_THROWS(addr.load<uint32_t>(4));
    }

    SECTION("get u64")
    {
        auto addr = ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f");
        REQUIRE(addr.load<uint64_t>(0) == 0x1011121314151617);
        REQUIRE(addr.load<uint64_t>(1) == 0x18191a1b1c1d1e1f);
        REQUIRE_THROWS(addr.load<uint64_t>(2));
    }

    SECTION("loopback recognition check")
    {
        REQUIRE(is_loopback(ipv6_address("::1")));
    }

    SECTION("multicast recognition check")
    {
        REQUIRE(is_multicast(ipv6_address("ff00::1")));
        REQUIRE(is_multicast(ipv6_address("ffff::1")));
        REQUIRE(!is_multicast(ipv6_address("fe00::1")));
        REQUIRE(!is_multicast(ipv6_address("::1")));
    }

    SECTION("link local recognition check")
    {
        REQUIRE(is_linklocal(ipv6_address("fe80::1")));
        REQUIRE(is_linklocal(ipv6_address("fe8f::1")));
        REQUIRE(is_linklocal(ipv6_address("fe90::1")));
        REQUIRE(is_linklocal(ipv6_address("fea0::1")));
        REQUIRE(is_linklocal(ipv6_address("feb0::1")));
        REQUIRE(!is_linklocal(ipv6_address("::1")));
        REQUIRE(!is_linklocal(ipv6_address("fe00::1")));
        REQUIRE(!is_linklocal(ipv6_address("fec0::1")));
    }

    SECTION("check comparison operators")
    {
        ipv6_address a("::1");
        ipv6_address b("ff::1");
        ipv6_address c = b;

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
        REQUIRE(to_string(ipv6_address("2001::1")) == "2001::1");
        REQUIRE(to_string(ipv6_address("fe80:0:0:0:0:0:0:1")) == "fe80::1");
        REQUIRE(
            to_string(ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f"))
            == "1011:1213:1415:1617:1819:1a1b:1c1d:1e1f");
        REQUIRE(to_string(ipv6_address("::1")) == "::1");
    }
}
