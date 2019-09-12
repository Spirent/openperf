#include <array>

#include "catch.hpp"

#include "spirent_pga/api.h"
#include "api_test.h"

TEST_CASE("fill functions", "[spirent-pga]")
{
    SECTION("implementations") {
        auto& functions = pga::functions::instance();
        constexpr uint16_t buffer_length = 128;
        std::array<uint32_t, buffer_length> buffer;

        /* Check that all functions generate valid fill data correctly */
        SECTION("constant") {
            unsigned const_tests = 0;
            for (auto instruction_set : { pga::instruction_set::type::SCALAR,
                                          pga::instruction_set::type::SSE2,
                                          pga::instruction_set::type::SSE4,
                                          pga::instruction_set::type::AVX,
                                          pga::instruction_set::type::AVX2,
                                          pga::instruction_set::type::AVX512SKX }) {

                auto fill_fn = pga::test::get_function(functions.fill_step_aligned_impl,
                                                       instruction_set);

                if (!(fill_fn && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                const_tests++;

                /* Fill the buffer with incorrect data before use */
                std::fill(std::begin(buffer), std::end(buffer), 0x55);

                fill_fn(buffer.data(), buffer.size(), 0x00, 0x00);

                /* Sum of all 0's is 0 */
                auto sum = std::accumulate(std::begin(buffer), std::end(buffer), 0UL);
                REQUIRE(sum == 0);

                /* The difference between 0's is 0 */
                std::array<uint32_t, buffer_length> difference;
                std::adjacent_difference(std::begin(buffer), std::end(buffer),
                                         std::begin(difference));
                /* As is there sum */
                auto adj_sum = std::accumulate(std::begin(difference) + 1,
                                               std::end(difference), 0UL);
                REQUIRE(adj_sum == 0);
            }
            REQUIRE(const_tests > 1);
        }

        SECTION("increment") {
            unsigned incr_tests = 0;
            for (auto instruction_set : { pga::instruction_set::type::SCALAR,
                                          pga::instruction_set::type::SSE2,
                                          pga::instruction_set::type::SSE4,
                                          pga::instruction_set::type::AVX,
                                          pga::instruction_set::type::AVX2,
                                          pga::instruction_set::type::AVX512SKX }) {

                auto fill_fn = pga::test::get_function(functions.fill_step_aligned_impl,
                                                       instruction_set);

                if (!(fill_fn && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                incr_tests++;

                /* Fill the buffer with incorrect data before use */
                std::fill(std::begin(buffer), std::end(buffer), 0x55);

                fill_fn(buffer.data(), buffer.size(), 0x00, 2);

                uint8_t* data = reinterpret_cast<uint8_t*>(buffer.data());
                constexpr uint16_t length = buffer_length * 4;

                /* First byte must be 0 */
                REQUIRE(data[0] == 0);

                /* The difference between each *byte* should be 2 */
                std::array<uint8_t, length> difference;
                std::adjacent_difference(data, data + length,
                                         std::begin(difference));
                /* So the sum is 2 * (length - 1) */
                auto adj_sum = std::accumulate(std::begin(difference) + 1,
                                               std::end(difference), 0);
                REQUIRE(adj_sum == (length - 1) * 2);
            }
            REQUIRE(incr_tests > 1);
        }

        SECTION("decrement") {
            unsigned decr_tests = 0;
            for (auto instruction_set : { pga::instruction_set::type::SCALAR,
                                          pga::instruction_set::type::SSE2,
                                          pga::instruction_set::type::SSE4,
                                          pga::instruction_set::type::AVX,
                                          pga::instruction_set::type::AVX2,
                                          pga::instruction_set::type::AVX512SKX }) {

                auto fill_fn = pga::test::get_function(functions.fill_step_aligned_impl,
                                                       instruction_set);

                if (!(fill_fn && pga::instruction_set::available(instruction_set))) {
                    continue;
                }

                decr_tests++;

                /* Fill the buffer with incorrect data before use */
                std::fill(std::begin(buffer), std::end(buffer), 0x55);

                fill_fn(buffer.data(), buffer.size(), 0x00, -2);

                uint8_t* data = reinterpret_cast<uint8_t*>(buffer.data());
                constexpr uint16_t length = buffer_length * 4;

                /* First byte must be 0 */
                REQUIRE(data[0] == 0);

                /* The difference between each *byte* should be 254 (e.g. -2) */
                std::array<uint8_t, length> difference;
                std::adjacent_difference(data, data + length,
                                         std::begin(difference));
                /* So the sum is 254 * (length - 1) */
                auto adj_sum = std::accumulate(std::begin(difference) + 1,
                                               std::end(difference), 0);
                REQUIRE(adj_sum == (length - 1) * 254);
            }
            REQUIRE(decr_tests > 1);
        }
    }
}
