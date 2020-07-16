
#include <algorithm>

#include "catch.hpp"
#include "memory/io_pattern.hpp"
#include "memory/generator.hpp"

TEST_CASE("Memory Generator Task", "[memory]")
{
    using namespace openperf::memory::internal;

    SECTION("index vector by pattern")
    {
        constexpr auto test_size = 10;

        SECTION("Pattern: SEQUENTIAL")
        {
            auto indexes =
                generator::fill_index_vector(test_size, io_pattern::SEQUENTIAL);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: REVERSE")
        {
            std::vector<unsigned> indexes;
            auto indexes =
                generator::fill_index_vector(test_size, io_pattern::SEQUENTIAL);
            std::reverse(indexes.begin(), indexes.end());

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: RANDOM")
        {
            std::vector<unsigned> indexes;
            auto indexes =
                generator::fill_index_vector(test_size, io_pattern::SEQUENTIAL);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));

            std::reverse(indexes.begin(), indexes.end());
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));
        }
    }
}