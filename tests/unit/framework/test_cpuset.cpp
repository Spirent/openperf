#include "catch.hpp"

#include "core/op_cpuset.hpp"

using cpuset = openperf::core::cpuset;

TEST_CASE("verify cpuset wrapper", "[cpuset]")
{
    SECTION("construct,")
    {
        auto a = cpuset();
        REQUIRE(a.count() == 0);

        auto b = cpuset("0xf");
        REQUIRE(b.count() == 4);

        REQUIRE_THROWS(cpuset("not a number"));
        REQUIRE_THROWS(cpuset("0xgarbage"));
    }

    SECTION("methods,")
    {
        auto a = cpuset();
        REQUIRE(a.to_string() == "0x0");

        a.set();
        REQUIRE(a.count() == a.size());

        a.reset();
        REQUIRE(a.count() == 0);

        a.set(1);
        REQUIRE(a.count() == 1);

        a.reset(1);
        REQUIRE(a.count() == 0);

        a.flip();
        REQUIRE(a.count() == a.size());
    }

    SECTION("operators,")
    {
        auto a = cpuset();
        a.set(0);

        REQUIRE(a == cpuset("0x1"));
        REQUIRE(a != cpuset("0x2"));

        auto b = cpuset();
        b.set(1);

        REQUIRE((a | b) == cpuset("0x3"));
        REQUIRE((a & b) == cpuset("0x0"));

        auto c = ~cpuset();
        REQUIRE(c.count() == c.size());
    }
}
