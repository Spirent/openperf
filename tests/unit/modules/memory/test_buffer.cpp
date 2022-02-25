#include <array>

#include "catch.hpp"

#include "memory/generator/buffer.hpp"

using namespace openperf::memory::generator;

TEST_CASE("memory buffer", "[memory]")
{
    auto constexpr buffer_size = 1048576;
    auto constexpr chunk_size = 1024;

    SECTION("sanity, ")
    {
        auto b = buffer(buffer_size);
        REQUIRE(b.data() != nullptr);
        REQUIRE(b.length() == buffer_size);
    }

    SECTION("readable, ")
    {
        auto b = buffer(buffer_size);
        auto scratch = std::array<std::byte, chunk_size>{};

        for (auto offset = 0; offset < buffer_size; offset += chunk_size) {
            REQUIRE(
                std::copy_n(b.data() + offset, chunk_size, std::begin(scratch))
                == std::end(scratch));
        }
    }

    SECTION("writable, ")
    {
        auto b = buffer(buffer_size);
        auto scratch = std::array<std::byte, chunk_size>{};
        scratch.fill(std::byte{0});

        for (auto offset = 0; offset < buffer_size; offset += chunk_size) {
            auto cursor = b.data() + offset;
            REQUIRE(std::copy_n(std::begin(scratch), chunk_size, cursor)
                    == cursor + chunk_size);
        }
    }
}
