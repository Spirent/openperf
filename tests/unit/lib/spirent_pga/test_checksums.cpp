#include <algorithm>
#include <array>
#include <vector>

#include "catch.hpp"

#include "spirent_pga/api.h"
#include "api_test.h"

uint16_t fold(uint32_t sum)
{
    auto tmp = (sum >> 16) + (sum & 0xffff);
    tmp += (tmp >> 16);
    return (static_cast<uint16_t>(tmp));
}

TEST_CASE("checksum functions", "[spirent-pga]")
{
    SECTION("implementations")
    {
        auto& functions = pga::functions::instance();

        SECTION("IPv4 header")
        {
            constexpr uint16_t ipv4_header_max_length = 60;
            constexpr uint16_t nb_headers = 32;
            /*
             * XXX: Using array here to work around issue with the Xcode.app
             * toolchain whining about destroying uint8_t[] values in vectors.
             */
            std::array<uint8_t[ipv4_header_max_length], nb_headers>
                ipv4_headers;
            std::vector<const uint8_t*> ipv4_header_ptrs(nb_headers);

            /* Fill in headers with (pseudo) random data */
            uint32_t seed = 0xffffffff;
            unsigned index = 0;
            std::for_each(std::begin(ipv4_headers),
                          std::end(ipv4_headers),
                          [&](auto& buffer) {
                              uint8_t* ptr = std::addressof(buffer[0]);
                              uint16_t ihl = (5 + (index % 11));
                              seed = scalar::fill_prbs_aligned(
                                  reinterpret_cast<uint32_t*>(ptr), ihl, seed);
                              ptr[0] = 0x4 << 4 | ihl;
                              index++;
                          });

            /* Generate vector of pointers for checksum functions */
            std::transform(
                std::begin(ipv4_headers),
                std::end(ipv4_headers),
                std::begin(ipv4_header_ptrs),
                [](auto& buffer) { return (std::addressof(buffer[0])); });

            SECTION("checksum")
            {
                auto scalar_fn = pga::test::get_function(
                    functions.checksum_ipv4_headers_impl,
                    pga::instruction_set::type::SCALAR);
                REQUIRE(scalar_fn != nullptr);

                std::vector<uint32_t> ref_checksums(nb_headers);

                scalar_fn(
                    ipv4_header_ptrs.data(), nb_headers, ref_checksums.data());

                unsigned vector_tests = 0;

                for (auto instruction_set :
                     pga::test::vector_instruction_sets()) {
                    auto vector_fn = pga::test::get_function(
                        functions.checksum_ipv4_headers_impl, instruction_set);

                    if (!(vector_fn
                          && pga::instruction_set::available(
                              instruction_set))) {
                        continue;
                    }

                    INFO("instruction set = "
                         << pga::instruction_set::to_string(instruction_set));

                    vector_tests++;

                    std::vector<uint32_t> checksums(nb_headers);
                    vector_fn(
                        ipv4_header_ptrs.data(), nb_headers, checksums.data());

                    REQUIRE(std::equal(std::begin(ref_checksums),
                                       std::end(ref_checksums),
                                       std::begin(checksums)));
                }
                REQUIRE(vector_tests > 0);
            }

            SECTION("pseudoheader checksum")
            {
                auto scalar_fn = pga::test::get_function(
                    functions.checksum_ipv4_pseudoheaders_impl,
                    pga::instruction_set::type::SCALAR);
                REQUIRE(scalar_fn != nullptr);

                std::vector<uint32_t> ref_checksums(nb_headers);

                scalar_fn(
                    ipv4_header_ptrs.data(), nb_headers, ref_checksums.data());

                unsigned vector_tests = 0;

                for (auto instruction_set :
                     pga::test::vector_instruction_sets()) {
                    auto vector_fn = pga::test::get_function(
                        functions.checksum_ipv4_pseudoheaders_impl,
                        instruction_set);

                    if (!(vector_fn
                          && pga::instruction_set::available(
                              instruction_set))) {
                        continue;
                    }

                    INFO("instruction set = "
                         << pga::instruction_set::to_string(instruction_set));

                    vector_tests++;

                    std::vector<uint32_t> checksums(nb_headers);
                    vector_fn(
                        ipv4_header_ptrs.data(), nb_headers, checksums.data());

                    REQUIRE(std::equal(std::begin(ref_checksums),
                                       std::end(ref_checksums),
                                       std::begin(checksums)));
                }

                REQUIRE(vector_tests > 0);
            }
        }

        SECTION("IPv6 header")
        {
            constexpr uint16_t ipv6_header_length = 40;
            constexpr uint16_t nb_headers = 32;
            /*
             * XXX: Using array here to work around issue with the Xcode.app
             * toolchain whining about destroying uint8_t[] values in vectors.
             */
            std::array<uint8_t[ipv6_header_length], nb_headers> ipv6_headers;
            std::vector<const uint8_t*> ipv6_header_ptrs(nb_headers);

            /* Fill in headers with (pseudo) random data */
            uint32_t seed = 0xffffffff;
            std::for_each(std::begin(ipv6_headers),
                          std::end(ipv6_headers),
                          [&](auto& buffer) {
                              uint8_t* ptr = std::addressof(buffer[0]);
                              uint16_t length = ipv6_header_length;
                              seed = pga_fill_prbs(&ptr, &length, 1, seed);
                          });

            /* Generate vector of pointers for checksum functions */
            std::transform(
                std::begin(ipv6_headers),
                std::end(ipv6_headers),
                std::begin(ipv6_header_ptrs),
                [](auto& buffer) { return (std::addressof(buffer[0])); });

            SECTION("pseudoheader checksum")
            {
                auto scalar_fn = pga::test::get_function(
                    functions.checksum_ipv6_pseudoheaders_impl,
                    pga::instruction_set::type::SCALAR);
                REQUIRE(scalar_fn != nullptr);

                std::vector<uint32_t> ref_checksums(nb_headers);

                scalar_fn(
                    ipv6_header_ptrs.data(), nb_headers, ref_checksums.data());

                unsigned vector_tests = 0;

                for (auto instruction_set :
                     pga::test::vector_instruction_sets()) {
                    auto vector_fn = pga::test::get_function(
                        functions.checksum_ipv6_pseudoheaders_impl,
                        instruction_set);

                    if (!(vector_fn
                          && pga::instruction_set::available(
                              instruction_set))) {
                        continue;
                    }

                    INFO("instruction set = "
                         << pga::instruction_set::to_string(instruction_set));

                    vector_tests++;

                    std::vector<uint32_t> checksums(nb_headers);
                    vector_fn(
                        ipv6_header_ptrs.data(), nb_headers, checksums.data());

                    REQUIRE(std::equal(std::begin(ref_checksums),
                                       std::end(ref_checksums),
                                       std::begin(checksums)));
                }

                REQUIRE(vector_tests > 0);
            }
        }

        SECTION("bulk checksums")
        {
            /* Create a buffer filled with (pseudo)random data */
            std::vector<uint32_t> buffer(512);
            auto prbs_fn =
                pga::test::get_function(functions.fill_prbs_aligned_impl,
                                        pga::instruction_set::type::SCALAR);
            prbs_fn(buffer.data(), buffer.size(), 0xffffffff);

            auto scalar_fn =
                pga::test::get_function(functions.checksum_data_aligned_impl,
                                        pga::instruction_set::type::SCALAR);
            REQUIRE(scalar_fn != nullptr);

            auto ref_sum = scalar_fn(buffer.data(), buffer.size());

            unsigned vector_tests = 0;

            for (auto instruction_set : pga::test::vector_instruction_sets()) {
                auto vector_fn = pga::test::get_function(
                    functions.checksum_data_aligned_impl, instruction_set);

                if (!(vector_fn
                      && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                INFO("instruction set = "
                     << pga::instruction_set::to_string(instruction_set));

                vector_tests++;

                auto sum = vector_fn(buffer.data(), buffer.size());

                /* The sums might not be equals, but their folded values should
                 * be */
                REQUIRE(fold(ref_sum) == fold(sum));
            }
            REQUIRE(vector_tests > 0);
        }
    }
}
