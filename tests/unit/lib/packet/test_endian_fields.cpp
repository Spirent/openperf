#include "catch.hpp"

#include "packet/type/endian.hpp"

template <size_t N, typename T>
void sanity_check_field(libpacket::type::endian::field<N>& lhs, T rhs)
{
    REQUIRE(rhs != 0);
    REQUIRE(std::is_pod_v<libpacket::type::endian::field<N>>);

    REQUIRE(lhs == rhs);

    REQUIRE(lhs.template load<T>() == rhs);
    REQUIRE(lhs != rhs + 1);
    REQUIRE(lhs <= rhs + 1);
    REQUIRE(lhs < rhs + 1);
    REQUIRE(lhs >= rhs - 1);
    REQUIRE(lhs > rhs - 1);
}

TEST_CASE("endian fields", "[libpacket]")
{

    SECTION("1 octet field, ")
    {
        constexpr size_t nb_octets = 1;
        auto test_value = uint8_t{0x12};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("2 octet field, ")
    {
        constexpr size_t nb_octets = 2;
        auto test_value = uint16_t{0x1234};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("3 octet field, ")
    {
        constexpr size_t nb_octets = 3;
        auto test_value = uint32_t{0x123456};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("4 octet field, ")
    {
        constexpr size_t nb_octets = 4;
        auto test_value = uint32_t{0x12345678};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("5 octet field, ")
    {
        constexpr size_t nb_octets = 5;
        auto test_value = uint64_t{0x1234567890};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("6 octet field, ")
    {
        constexpr size_t nb_octets = 6;
        auto test_value = uint64_t{0x1234567890ab};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("7 octet field, ")
    {
        constexpr size_t nb_octets = 7;
        auto test_value = uint64_t{0x1234567890abcd};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("8 octet field, ")
    {
        constexpr size_t nb_octets = 8;
        auto test_value = uint64_t{0x1234567890abcdef};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);
        sanity_check_field(x, test_value);
    }

    SECTION("16 octet field, ")
    {
        constexpr size_t nb_octets = 16;
        auto test_value = std::initializer_list<uint8_t>{0x01,
                                                         0x02,
                                                         0x03,
                                                         0x04,
                                                         0x05,
                                                         0x06,
                                                         0x07,
                                                         0x08,
                                                         0x09,
                                                         0x0a,
                                                         0x0b,
                                                         0x0c,
                                                         0x0e,
                                                         0x0e,
                                                         0x0f,
                                                         0x00};

        auto x = libpacket::type::endian::field<nb_octets>(test_value);
        REQUIRE(sizeof(x) == nb_octets);

        /*
         * No sanity check because there is no native type that
         * can hold 16 octets.
         */
    }
}
