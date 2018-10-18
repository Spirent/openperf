#include "catch.hpp"

#include "net/mac_address.h"

using mac_address = icp::net::mac_address;

TEST_CASE("mac_address functionality checks", "[mac_address]")
{
    SECTION("construction by string") {
        /* valid */
        REQUIRE(mac_address("01.02.03.04.05.06") == mac_address({0x01, 0x02, 0x03, 0x04, 0x05, 0x06}));
        REQUIRE(mac_address("de-ad-be-ef-00-01") == mac_address({0xde, 0xad, 0xbe, 0xef, 0x00, 0x01}));
        REQUIRE(mac_address("5c:a1:ab:1e:ff:fe") == mac_address({0x5c, 0xa1, 0xab, 0x1e, 0xff, 0xfe}));

        /* not valid */
        REQUIRE_THROWS(mac_address("01-02-03-04"));           /* too short */
        REQUIRE_THROWS(mac_address("01-02-03-04-05-06-07"));  /* too long */
        REQUIRE_THROWS(mac_address("01:02:03:04:05:300"));    /* value too big */
    }

    SECTION("access by index") {
        mac_address test{0xfe, 0x00, 0x01, 0xc0, 0xff, 0xfe};

        REQUIRE(test[0] == 0xfe);
        REQUIRE(test[1] == 0x00);
        REQUIRE(test[2] == 0x01);
        REQUIRE(test[3] == 0xc0);
        REQUIRE(test[4] == 0xff);
        REQUIRE(test[5] == 0xfe);
        REQUIRE_THROWS(test[6]);
    }

    SECTION("broadcast recognition") {
        REQUIRE(mac_address("ff:ff:ff:ff:ff:ff").is_broadcast());
        REQUIRE(!mac_address({0, 0, 0, 0, 0, 1}).is_broadcast());
    }

    SECTION("multicast group recognition") {
        mac_address mcast("01:ff:fe:00:00:01");
        mac_address ucast("00:ff:fe:00:00:01");

        REQUIRE(mcast.is_multicast());
        REQUIRE(!mcast.is_unicast());
        REQUIRE(!ucast.is_multicast());
        REQUIRE(ucast.is_unicast());
    }

    SECTION("local admin recognition") {
        mac_address local("02-a-b-c-d-e");
        mac_address global("5-a-b-c-d-e");

        REQUIRE(local.is_local_admin());
        REQUIRE(!local.is_globally_unique());
        REQUIRE(!global.is_local_admin());
        REQUIRE(global.is_globally_unique());
    }

    SECTION("check comparison operators") {
        mac_address a("00-00-00-00-00-01");
        mac_address b("a-a-a-a-a-a");
        mac_address c("a-a-a-a-a-a");

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b <= c);
        REQUIRE(b > a);
        REQUIRE(b >= a);
        REQUIRE(b >= c);
        REQUIRE(b == c);
        REQUIRE(a != b);
    }
}
