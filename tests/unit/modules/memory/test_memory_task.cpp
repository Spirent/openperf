
#include <algorithm>

#include "catch.hpp"
#include "memory/io_pattern.hpp"

namespace openperf::memory::internal {
void index_vector(std::vector<unsigned>& indexes,
                  size_t size,
                  io_pattern pattern);
}

TEST_CASE("Memory Generator Task", "[memory]")
{
    using namespace openperf::memory::internal;

    SECTION("index vector by pattern")
    {
        constexpr auto test_size = 10;

        SECTION("Pattern: SEQUENTIAL")
        {
            std::vector<unsigned> indexes;
            index_vector(indexes, test_size, io_pattern::SEQUENTIAL);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: REVERSE")
        {
            std::vector<unsigned> indexes;
            index_vector(indexes, test_size, io_pattern::REVERSE);
            std::reverse(indexes.begin(), indexes.end());

            REQUIRE(indexes.size() == test_size);
            REQUIRE(std::is_sorted(indexes.begin(), indexes.end()));
        }

        SECTION("Pattern: RANDOM")
        {
            std::vector<unsigned> indexes;
            index_vector(indexes, test_size, io_pattern::RANDOM);

            REQUIRE(indexes.size() == test_size);
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));

            std::reverse(indexes.begin(), indexes.end());
            REQUIRE(!std::is_sorted(indexes.begin(), indexes.end()));
        }
    }
}