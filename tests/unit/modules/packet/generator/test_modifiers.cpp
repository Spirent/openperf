#include <numeric>
#include <iostream>
#include "catch.hpp"

#include "packet/generator/traffic/modifier.hpp"

using namespace openperf::packet::generator::traffic::modifier;
using namespace libpacket::type;

template <typename Field>
void test_modifier_sequence_skips(Field first,
                                  unsigned count,
                                  std::vector<Field>& skips)
{
    auto config =
        sequence_config<Field>{.skip = skips, .first = first, .count = count};

    auto rng = to_range(config);

    REQUIRE(ranges::count_if(rng, [](const auto&) { return (true); }) == count);

    /* none of the skips should be in the generated range */
    std::for_each(std::begin(skips), std::end(skips), [&](const auto& skip) {
        REQUIRE(ranges::count(rng, skip) == 0);
    });
}

template <typename Field>
void test_modifier_sequence_stepped(Field first, unsigned count, unsigned step)
{
    auto config = sequence_config<Field>{
        .first = first, .last = first + (count * step), .count = count};

    auto rng = to_range(config);

    REQUIRE(ranges::count_if(rng, [](const auto&) { return (true); }) == count);

    /* Make sure the difference between steps is correct */
    auto common = rng | ranges::views::common;
    auto deltas = std::vector<Field>{};
    std::adjacent_difference(
        ranges::begin(common), ranges::end(common), std::back_inserter(deltas));

    std::for_each(std::next(std::begin(deltas)),
                  std::end(deltas),
                  [&](const auto& item) { REQUIRE(item == Field(step)); });
}

template <typename Field>
void test_modifier_sequence_permute(Field first, unsigned count)
{
    auto config1 = sequence_config<Field>{
        .first = first,
        .count = count,
        .permute = false,
    };

    auto config2 = sequence_config<Field>{
        .first = first,
        .count = count,
        .permute = true,
    };

    auto rng1 = to_range(config1);
    auto rng2 = to_range(config2);

    REQUIRE(ranges::is_sorted(rng1));
    REQUIRE(!ranges::is_sorted(rng2));
    REQUIRE(!ranges::equal(rng1, rng2));
}

template <typename Field>
void test_modifier_list(const std::vector<Field>& items)
{
    auto config = list_config<Field>{.items = items};
    auto rng1 = to_range(config);
    REQUIRE(ranges::equal(rng1, ranges::views::all(items)));

    config.permute = true;
    auto rng2 = to_range(config);
    REQUIRE(!ranges::equal(rng2, ranges::views::all(items)));
}

TEST_CASE("packet generator modifiers", "[packet_generator]")
{
    SECTION("uint32_t")
    {
        auto skips = std::vector<uint32_t>{2, 3, 4};
        test_modifier_sequence_skips<uint32_t>(1, 10, skips);
        test_modifier_sequence_stepped<uint32_t>(1, 20, 2);
        test_modifier_sequence_permute<uint32_t>(48, 14);

        test_modifier_list(std::vector<uint32_t>{12, 45, 90, 34, 26});
    }

    SECTION("ipv4 address")
    {
        auto skips = std::vector<ipv4_address>{"198.18.1.2", "198.18.1.3"};
        test_modifier_sequence_skips<ipv4_address>("198.18.1.0", 4, skips);
        test_modifier_sequence_stepped<ipv4_address>("224.0.0.1", 20, 3);
        test_modifier_sequence_permute<ipv4_address>("198.51.100.1", 11);

        test_modifier_list(std::vector<ipv4_address>{
            "203.0.113.1", "203.0.113.5", "203.0.113.99", "203.0.113.255"});
    }

    SECTION("ipv6 address")
    {
        auto skips = std::vector<ipv6_address>{"2001:db8::2", "2001:db8::3"};
        test_modifier_sequence_skips<ipv6_address>("2001:db8::1", 4, skips);
        test_modifier_sequence_stepped<ipv6_address>("ff00::1", 20, 4);
        test_modifier_sequence_permute<ipv6_address>("2001:dba::1", 11);

        test_modifier_list(std::vector<ipv6_address>{
            "2001:dbb::1", "2001:dbb::11", "2001:dbb::111", "2001:dbb::1111"});
    }

    SECTION("mac address")
    {
        /*
         * Note: these skips aren't actually in the sequence; we're testing
         * a configuration error. :)
         */
        auto skips =
            std::vector<mac_address>{"10:94:00:01:00:aa", "10:94:00:01:00:ab"};
        test_modifier_sequence_skips<mac_address>(
            "10:94:00:01:00:a0", 4, skips);
        test_modifier_sequence_stepped<mac_address>("ff:00:00:00:00:01", 20, 5);
        test_modifier_sequence_permute<mac_address>("00:00:01:00:00:01", 100);

        test_modifier_list(std::vector<mac_address>{"00:00:01:00:00:01",
                                                    "00:00:01:00:00:01",
                                                    "00:00:02:00:00:02",
                                                    "00:00:03:00:00:03",
                                                    "00:00:05:00:00:05"});
    }
}
