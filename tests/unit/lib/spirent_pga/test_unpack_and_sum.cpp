#include <array>

#include "catch.hpp"

#include "spirent_pga/api.h"
#include "api_test.h"

TEST_CASE("unpack and sum", "[spirent-pga]")
{
    SECTION("implementations")
    {
        auto& functions = pga::functions::instance();
        constexpr uint16_t buffer_length = 64;

        std::array<uint32_t, buffer_length> buffer;
        std::fill_n(buffer.data(), buffer.size(), 0xec2);

        /*
         * The value in the buffer represents a packed counter.  Each
         * hex-digit is an index into a counter space.  The mask below
         * captures the value of each digit, e.g.
         * (0xf00 & 0xec2) >> 2 = 0xe
         * (0x070 & 0xec2) >> 1 = 0x4
         * (0x003 & 0xec2) >> 0 = 0x2
         */
        std::array<uint32_t, 3> masks = {0xf00, 0x70, 0x3};
        std::array<uint64_t, 16> counters_a;
        std::array<uint64_t, 8> counters_b;
        std::array<uint64_t, 4> counters_c;

        std::array<uint64_t*, 3> counters = {
            counters_a.data(), counters_b.data(), counters_c.data()};

        /* Check that all functions generate valid sums */
        unsigned tests = 0;
        for (auto instruction_set : pga::test::instruction_sets()) {
            auto unpack_and_sum_fn = pga::test::get_function(
                functions.unpack_and_sum_indexicals_impl, instruction_set);

            if (!(unpack_and_sum_fn
                  && pga::instruction_set::available(instruction_set))) {
                continue;
            }

            INFO("instruction set = "
                 << pga::instruction_set::to_string(instruction_set));

            tests++;

            /* Zero out counters */
            std::fill_n(counters_a.data(), counters_a.size(), 0);
            std::fill_n(counters_b.data(), counters_b.size(), 0);
            std::fill_n(counters_c.data(), counters_c.size(), 0);

            unpack_and_sum_fn(buffer.data(),
                              buffer.size(),
                              masks.data(),
                              masks.size(),
                              counters.data());

            REQUIRE(counters_a[0xe] == buffer_length);
            REQUIRE(std::accumulate(counters_a.data(),
                                    counters_a.data() + counters_a.size(),
                                    0)
                    == buffer_length);

            REQUIRE(counters_b[0x4] == buffer_length);
            REQUIRE(std::accumulate(counters_b.data(),
                                    counters_b.data() + counters_b.size(),
                                    0)
                    == buffer_length);

            REQUIRE(counters_c[0x2] == buffer_length);
            REQUIRE(std::accumulate(counters_c.data(),
                                    counters_c.data() + counters_c.size(),
                                    0)
                    == buffer_length);
        }

        REQUIRE(tests > 1);
    }
}
