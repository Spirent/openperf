#include "catch.hpp"

#include "net/ipv6_address.hpp"
#include <arpa/inet.h>

using namespace openperf::net;

TEST_CASE("ipv6_address functionality checks", "[ipv6_address]")
{
    SECTION("constructor functionality checks")
    {
        // default constructor is all zero
        REQUIRE(ipv6_address().data()
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
        REQUIRE_THROWS(ipv6_address{
            0x20, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0});
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
        REQUIRE_THROWS(test[16]);
    }

    SECTION("loopback recognition check")
    {
        REQUIRE(ipv6_address("::1").is_loopback());
    }

    SECTION("multicast recognition check")
    {
        REQUIRE(ipv6_address("ff00::1").is_multicast());
        REQUIRE(ipv6_address("ffff::1").is_multicast());
        REQUIRE(!ipv6_address("fe00::1").is_multicast());
        REQUIRE(!ipv6_address("::1").is_multicast());
    }

    SECTION("link local recognition check")
    {
        REQUIRE(ipv6_address("fe80::1").is_linklocal());
        REQUIRE(ipv6_address("fe8f::1").is_linklocal());
        REQUIRE(ipv6_address("fe90::1").is_linklocal());
        REQUIRE(ipv6_address("fea0::1").is_linklocal());
        REQUIRE(ipv6_address("feb0::1").is_linklocal());
        REQUIRE(!ipv6_address("::1").is_linklocal());
        REQUIRE(!ipv6_address("fe00::1").is_linklocal());
        REQUIRE(!ipv6_address("fec0::1").is_linklocal());
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

    SECTION("prefix mask")
    {
        REQUIRE(ipv6_address::make_prefix_mask(1) == ipv6_address("8000::0"));
        REQUIRE(ipv6_address::make_prefix_mask(2) == ipv6_address("c000::"));
        REQUIRE(ipv6_address::make_prefix_mask(3) == ipv6_address("e000::"));
        REQUIRE(ipv6_address::make_prefix_mask(4) == ipv6_address("f000::"));
        REQUIRE(ipv6_address::make_prefix_mask(8) == ipv6_address("ff00::"));
        REQUIRE(ipv6_address::make_prefix_mask(16) == ipv6_address("ffff::"));
        REQUIRE(ipv6_address::make_prefix_mask(32)
                == ipv6_address("ffff:ffff::"));
        REQUIRE(ipv6_address::make_prefix_mask(64)
                == ipv6_address("ffff:ffff:ffff:ffff::"));
        REQUIRE(ipv6_address::make_prefix_mask(124)
                == ipv6_address("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff0"));
        REQUIRE(ipv6_address::make_prefix_mask(125)
                == ipv6_address("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff8"));
        REQUIRE(ipv6_address::make_prefix_mask(126)
                == ipv6_address("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc"));
        REQUIRE(ipv6_address::make_prefix_mask(127)
                == ipv6_address("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe"));
        REQUIRE(ipv6_address::make_prefix_mask(128)
                == ipv6_address("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
    }

    SECTION("logical and")
    {
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(128))
                == ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(120))
                == ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e00"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(112))
                == ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:0"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(64))
                == ipv6_address("1011:1213:1415:1617::"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(32))
                == ipv6_address("1011:1213::"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(16))
                == ipv6_address("1011::"));
        REQUIRE((ipv6_address("1011:1213:1415:1617:1819:1a1b:1c1d:1e1f")
                 & ipv6_address::make_prefix_mask(8))
                == ipv6_address("1000::"));
    }
}
