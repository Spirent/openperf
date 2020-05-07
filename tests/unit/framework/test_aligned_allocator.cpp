#include "catch.hpp"

#include "memory/aligned_allocator.hpp"

template <typename T, size_t Alignment>
using aligned_vector =
    std::vector<T, openperf::memory::aligned_allocator<T, Alignment>>;

TEST_CASE("verify aligned allocator", "[aligned_allocator]")
{
    SECTION("can allocate/deallocate,")
    {
        auto aa = openperf::memory::aligned_allocator<int, 64>();

        auto thing1 = aa.allocate(1);
        REQUIRE(thing1);

        aa.deallocate(thing1, sizeof(thing1));

        auto thing2 = aa.allocate(0);
        REQUIRE(thing2 == nullptr);

        REQUIRE_THROWS(aa.allocate(std::numeric_limits<size_t>::max()));
    }

    SECTION("can use in stl container,")
    {
        constexpr auto test_items = 256;
        constexpr auto test_alignment = 32;
        auto test_vector = aligned_vector<uint8_t, test_alignment>{};

        auto values = std::vector<uint8_t>{};
        values.reserve(test_items);
        std::iota(std::begin(values), std::end(values), 0x0);
        std::for_each(std::begin(values), std::end(values), [&](auto&& item) {
            test_vector.push_back(item);
            REQUIRE((reinterpret_cast<uintptr_t>(test_vector.data())
                     & (test_alignment - 1))
                    == 0);
        });
    }
}
