#include "catch.hpp"

#include "modules/dynamic/threshold.hpp"

#include <random>

using namespace openperf::dynamic;

constexpr auto equal_items = 10;
constexpr auto greater_items = 30;
constexpr auto less_items = 50;

threshold make_threshold(double value, comparator cmp)
{
    std::random_device rdev;
    std::mt19937 mt(rdev());
    std::uniform_real_distribution<> dist(value - 10.0, value);

    auto th = threshold{value, cmp};

    for (int i = 0; i < equal_items; i++) th.append(value);
    for (int i = 0; i < less_items; i++) th.append(dist(mt));
    for (int i = 0; i < greater_items; i++) th.append(dist(mt) + 10.0);

    return th;
}

TEST_CASE("Test Dynamic Results Framework", "[dynamic results]")
{
    SECTION("check threshold EQUAL")
    {
        auto th = make_threshold(10.0, comparator::EQUAL);

        REQUIRE(th.count() == equal_items + greater_items + less_items);
        REQUIRE(th.trues() == equal_items);
        REQUIRE(th.falses() == greater_items + less_items);
    }

    SECTION("check threshold GREATER_THAN")
    {
        auto th = make_threshold(10.0, comparator::GREATER_THAN);

        REQUIRE(th.count() == equal_items + greater_items + less_items);
        REQUIRE(th.trues() == greater_items);
        REQUIRE(th.falses() == equal_items + less_items);
    }

    SECTION("check threshold GREATER_OR_EQUAL")
    {
        auto th = make_threshold(10.0, comparator::GREATER_OR_EQUAL);

        REQUIRE(th.count() == equal_items + greater_items + less_items);
        REQUIRE(th.trues() == greater_items + equal_items);
        REQUIRE(th.falses() == less_items);
    }

    SECTION("check threshold LESS_THAN")
    {
        auto th = make_threshold(10.0, comparator::LESS_THAN);

        REQUIRE(th.count() == equal_items + greater_items + less_items);
        REQUIRE(th.trues() == less_items);
        REQUIRE(th.falses() == equal_items + greater_items);
    }

    SECTION("check threshold LESS_OR_EQUAL")
    {
        auto th = make_threshold(10.0, comparator::LESS_OR_EQUAL);

        REQUIRE(th.count() == equal_items + greater_items + less_items);
        REQUIRE(th.trues() == less_items + equal_items);
        REQUIRE(th.falses() == greater_items);
    }
}
