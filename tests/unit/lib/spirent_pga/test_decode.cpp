#include <array>

#include "catch.hpp"

#include "spirent_pga/api.h"

/*
 * Verify that we can decode data captured from Spirent hardware.
 * Note: the signature encode and decode tests verify that the decoded
 * data matches the encoded data and that all implementations match each
 * other. Hence, this single test is sufficient to verify that all of the
 * encode/decode functions work correctly (via transitivity).
 */
TEST_CASE("FPGA decode", "[spirent-pga]")
{
    constexpr uint8_t sig1[] = {0xe7, 0x0a, 0xfe, 0x2f, 0x4a, 0x96, 0x3d,
                                0xa5, 0x34, 0x27, 0x5e, 0xd9, 0xab, 0x7f,
                                0xb0, 0x5b, 0x7d, 0x5e, 0x99, 0xcf};
    constexpr uint8_t sig2[] = {0xe6, 0x7b, 0x20, 0x03, 0x92, 0xef, 0xff,
                                0x1a, 0xff, 0xf5, 0xd9, 0x34, 0x42, 0xd2,
                                0xd5, 0x0a, 0x36, 0x87, 0x2c, 0x93};
    constexpr uint8_t sig3[] = {0xe5, 0xe9, 0x42, 0x76, 0xfa, 0x65, 0xb8,
                                0xda, 0xa3, 0x82, 0x51, 0x02, 0x28, 0x75,
                                0x7b, 0xf8, 0xe2, 0x09, 0x52, 0xa4};

    constexpr auto nb_sigs = 3;
    auto signatures = std::array<const uint8_t*, nb_sigs>{sig1, sig2, sig3};

    auto matches = std::array<int, nb_sigs>{};

    auto nb_matches =
        pga_signatures_crc_filter(signatures.data(), nb_sigs, matches.data());

    REQUIRE(nb_matches == nb_sigs);

    auto stream_ids = std::array<uint32_t, nb_sigs>{};
    auto sequence_numbers = std::array<uint32_t, nb_sigs>{};
    auto timestamps = std::array<uint64_t, nb_sigs>{};
    auto flags = std::array<int, nb_sigs>{};

    auto nb_decodes = pga_signatures_decode(signatures.data(),
                                            nb_sigs,
                                            stream_ids.data(),
                                            sequence_numbers.data(),
                                            timestamps.data(),
                                            flags.data());

    REQUIRE(nb_decodes == nb_sigs);
}
