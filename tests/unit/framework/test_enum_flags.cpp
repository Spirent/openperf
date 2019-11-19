#include "catch.hpp"

#include "utils/enum_flags.h"

enum class test_enums {
    A = (1 << 0),
    B = (1 << 1),
    C = (1 << 2),
    D = (1 << 3),
};

declare_enum_flags(test_enums);

TEST_CASE("enums flags", "[enum flags]")
{
    using enum_type = std::underlying_type_t<test_enums>;
    auto mask = icp::utils::bit_flags<test_enums>(std::numeric_limits<enum_type>::max());
    auto enum_a = icp::utils::enumerator<test_enums>(test_enums::A);

    SECTION("test operator &") {
        auto a_and_b = test_enums::A & test_enums::B;
        REQUIRE(!a_and_b);

        auto enum_a_and_b = enum_a & test_enums::B;
        REQUIRE(a_and_b == enum_a_and_b);
        REQUIRE(!enum_a_and_b);

        auto a_and_mask = test_enums::A & mask;
        REQUIRE(a_and_mask);

        auto enum_a_and_mask = enum_a & mask;
        REQUIRE(enum_a_and_mask);

        auto a_and_b_and_mask = a_and_b & mask;
        REQUIRE(!a_and_b_and_mask);

        auto mask_and_b = mask & test_enums::B;
        REQUIRE(mask_and_b);
    }

    SECTION("test operator |") {
        auto a_or_c = test_enums::A | test_enums::C;
        REQUIRE(a_or_c);
        REQUIRE(a_or_c.value == 5);

        auto enum_a_or_c = enum_a | test_enums::C;
        REQUIRE(enum_a_or_c == a_or_c);

        auto a_or_mask = test_enums::A | mask;
        REQUIRE(a_or_mask == mask);
    }

    SECTION("test operator ^") {
        auto a_xor_d = test_enums::A ^ test_enums::D;
        REQUIRE(a_xor_d);
        REQUIRE(a_xor_d.value == 9);

        auto enum_a_xor_d = enum_a ^ test_enums::D;
        REQUIRE(enum_a_xor_d == a_xor_d);

        auto a_xor_mask = test_enums::A ^ mask;
        REQUIRE(a_xor_mask != mask);
        REQUIRE(a_xor_mask.value == (std::numeric_limits<enum_type>::max() ^ 1));

        auto a_xor_d_xor_mask = a_xor_d ^ mask;
        REQUIRE(a_xor_d_xor_mask != a_xor_d);
        REQUIRE(a_xor_d_xor_mask != mask);
    }

    SECTION("test operator ~") {
        auto not_a = ~test_enums::A;
        REQUIRE(not_a.value == (std::numeric_limits<unsigned>::max() ^ 1));

        auto not_a_or_b = ~(test_enums::A | test_enums::B);
        REQUIRE(not_a_or_b.value == (std::numeric_limits<unsigned>::max() ^ (1 | 2)));
    }
}
