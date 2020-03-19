#include "catch.hpp"

#include "packet/type/endian.hpp"

TEST_CASE("endian order", "[libpacket]")
{
    SECTION("host is big endian, ")
    {
        using namespace libpacket::type::endian;
        REQUIRE(order::native == order::little);
        REQUIRE(order::native != order::big);
        REQUIRE(order::little != order::big);
    }
}
