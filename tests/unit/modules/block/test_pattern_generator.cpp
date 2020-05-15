#include <chrono>

#include "catch.hpp"

#include "block/pattern_generator.hpp"

using namespace openperf::block::worker;

TEST_CASE("pattern_generator", "[block]")
{
    SECTION("reset, ")
    {
        auto pg = pattern_generator();

        SECTION("success, ")
        {
            REQUIRE_NOTHROW(pg.reset(2, 10, generation_pattern::REVERSE));
            REQUIRE_NOTHROW(pg.reset(-5, -2, generation_pattern::SEQUENTIAL));
        }

        SECTION("exception, ")
        {
            REQUIRE_THROWS(pg.reset(0, 0, generation_pattern::RANDOM));
            REQUIRE_THROWS(pg.reset(0, -1, generation_pattern::RANDOM));
        }
    }

    SECTION("pattern blank, ")
    {
        auto pg = pattern_generator();

        SECTION("zero sequece, ")
        {
            for (size_t i = 0; i < 10; i++) { REQUIRE(pg.generate() == 0); }
        }
    }

    SECTION("pattern RANDOM, ")
    {
        auto pg = pattern_generator();

        auto verify = [&](off_t min, off_t max, off_t value) {
            REQUIRE(value >= min);
            REQUIRE(value < max);
        };

        auto test = [&](off_t min, off_t max) {
            pg.reset(min, max, generation_pattern::RANDOM);
            for (off_t i = min; i < max; i++) {
                verify(min, max, pg.generate());
            }
        };

        SECTION("zero sequece, ") { test(0, 1); }

        SECTION("positive numbers, ") { test(0, 10); }

        SECTION("negative numbers, ") { test(-15, -10); }
    }

    SECTION("pattern SEQUENTIAL, ")
    {
        auto pg = pattern_generator();

        auto test = [&](off_t min, off_t max) {
            pg.reset(min, max, generation_pattern::SEQUENTIAL);
            for (off_t i = 0; i < (max - min) * 2; i++) {
                REQUIRE(pg.generate() == i % (max - min) + min);
            }
        };

        SECTION("zero sequece, ") { test(0, 1); }

        SECTION("positive numbers, ") { test(0, 10); }

        SECTION("negative numbers, ") { test(-15, -10); }
    }

    SECTION("pattern REVERSE, ")
    {
        auto pg = pattern_generator();

        auto test = [&](off_t min, off_t max) {
            pg.reset(min, max, generation_pattern::REVERSE);
            for (off_t i = 0; i < (max - min) * 2; i++) {
                REQUIRE(pg.generate() == (max - 1 - i % (max - min)));
            }
        };

        SECTION("zero sequece, ") { test(0, 1); }

        SECTION("positive numbers, ") { test(0, 10); }

        SECTION("negative numbers, ") { test(-15, -10); }
    }
}
