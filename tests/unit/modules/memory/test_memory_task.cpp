
#include <algorithm>

#include "catch.hpp"
#include "memory/io_pattern.hpp"

namespace openperf::memory::internal {
std::vector<unsigned> index_vector(size_t size, io_pattern pattern);
}

TEST_CASE("Memory Generator Task", "[memory]")
{
    using namespace openperf::memory::internal;

    SECTION("index vector by pattern")
    {
        constexpr auto test_size = 10;

        SECTION("Pattern: SEQUENTIAL")
        {
            auto v_idx = index_vector(test_size, io_pattern::SEQUENTIAL);

            REQUIRE(v_idx.size() == test_size);
            REQUIRE(std::is_sorted(v_idx.begin(), v_idx.end()));
        }

        SECTION("Pattern: REVERSE")
        {
            auto v_idx = index_vector(test_size, io_pattern::REVERSE);
            std::reverse(v_idx.begin(), v_idx.end());

            REQUIRE(v_idx.size() == test_size);
            REQUIRE(std::is_sorted(v_idx.begin(), v_idx.end()));
        }

        SECTION("Pattern: RANDOM")
        {
            auto v_idx = index_vector(test_size, io_pattern::RANDOM);

            REQUIRE(v_idx.size() == test_size);
            REQUIRE(!std::is_sorted(v_idx.begin(), v_idx.end()));

            std::reverse(v_idx.begin(), v_idx.end());
            REQUIRE(!std::is_sorted(v_idx.begin(), v_idx.end()));
        }
    }
}