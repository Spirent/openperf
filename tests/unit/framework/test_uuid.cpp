#include <iostream>
#include <set>

#include "catch.hpp"

#include "core/op_uuid.hpp"

using uuid = openperf::core::uuid;

TEST_CASE("uuid constructors", "[uuid]")
{
    SECTION("can construct nil UUID") {
        uuid u;
        for (size_t i = 0; i < 15; i++) {
            REQUIRE(u[i] == 0);
        }

        SECTION("can convert to string correctly") {
            REQUIRE(to_string(u) == "00000000-0000-0000-0000-000000000000");
        }
    }

    SECTION("can construct UUID from string") {
        uuid u("123e4567-e89b-12d3-a456-426655440000");
        REQUIRE(u[0]  == 0x12);
        REQUIRE(u[1]  == 0x3e);
        REQUIRE(u[2]  == 0x45);
        REQUIRE(u[3]  == 0x67);
        REQUIRE(u[4]  == 0xe8);
        REQUIRE(u[5]  == 0x9b);
        REQUIRE(u[6]  == 0x12);
        REQUIRE(u[7]  == 0xd3);
        REQUIRE(u[8]  == 0xa4);
        REQUIRE(u[9]  == 0x56);
        REQUIRE(u[10] == 0x42);
        REQUIRE(u[11] == 0x66);
        REQUIRE(u[12] == 0x55);
        REQUIRE(u[13] == 0x44);
        REQUIRE(u[14] == 0x00);
        REQUIRE(u[15] == 0x00);

        SECTION("can convert to string correctly") {
            REQUIRE(openperf::core::to_string(u) == "123e4567-e89b-12d3-a456-426655440000");
        }

        SECTION("can reject obviously bad strings") {
            REQUIRE_THROWS(new uuid("not-valid"));
            REQUIRE_THROWS(new uuid("123e4567-e89b-12d3-a456-4266554400"));      /* too short */
            REQUIRE_THROWS(new uuid("123e4567-e89b-12d3-a456-42665544000000"));  /* too long */
        }
    }

    SECTION("can construct UUID from bytes") {
        uint8_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        uuid u(data);

        for (size_t i = 0; i < 15; i++) {
            REQUIRE(u[i] == data[i]);
        }

        SECTION("can access raw data") {
            auto p = u.data();
            for (size_t i = 0; i < 15; i++) {
                REQUIRE(p[i] == data[i]);
            }
        }

        SECTION("can convert to string correctly") {
            REQUIRE(openperf::core::to_string(u) == "00112233-4455-6677-8899-aabbccddeeff");
        }
    }

    SECTION("can construct random UUIDs") {
        std::set<std::string> uuids;

        for (size_t i = 0; i < 100; i++) {
            auto u = uuid::random();

            /* some bits should be fixed in random UUIDs, so check those */
            REQUIRE((u[6] >> 4) == 0x04);  /* version */
            REQUIRE((u[8] >> 6) == 0x01);  /* variant */

            /* but we should not see any duplicates */
            auto s = openperf::core::to_string(u);
            REQUIRE(uuids.find(s) == uuids.end());
            uuids.insert(s);
        }
    }
}
