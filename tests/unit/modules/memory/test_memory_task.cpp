
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
            std::vector<unsigned> indexes;
            generator::update_index_vector(
                indexes, test_size, io_pattern::SEQUENTIAL);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: REVERSE")
        {
            std::vector<unsigned> indexes;
            generator::update_index_vector(
                indexes, test_size, io_pattern::REVERSE);
            std::reverse(indexes.begin(), indexes.end());

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: RANDOM")
        {
            std::vector<unsigned> indexes;
            generator::update_index_vector(
                indexes, test_size, io_pattern::RANDOM);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));

            std::reverse(indexes.begin(), indexes.end());
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));
        }
    }
}