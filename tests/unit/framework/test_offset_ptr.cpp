#include <iostream>
#include <numeric>
#include <string>

#include "catch.hpp"

#include "memory/offset_ptr.hpp"

struct test_struct
{
    int foo;
    double bar;
    std::string baz;
};

TEST_CASE("basic functionality for offset_ptr", "[offset_ptr]")
{
    SECTION("default constructor works, ")
    {
        openperf::memory::offset_ptr<int> x;

        SECTION("empty pointer equals null,") { REQUIRE(x == nullptr); }

        SECTION("empty pointer is false,")
        {
            REQUIRE(bool(x) == false);
            REQUIRE(!x);
        }

        SECTION("assignment works,")
        {
            int test = 5;
            x = &test;
            REQUIRE(*x == test);
            REQUIRE(x == &test);
        }
    }

    SECTION("pointer constructor works,")
    {
        auto x = openperf::memory::offset_ptr<char>(new auto('x'));

        SECTION("can get constructed value,") { REQUIRE(*x == 'x'); }

        SECTION("can get pointer value,")
        {
            auto x_ = x.get();
            REQUIRE(x == x_);
        }

        SECTION("can construct offset_ptr from an offset_ptr,")
        {
            auto y = openperf::memory::offset_ptr<char>(x);
            REQUIRE(y == x);
            REQUIRE(*y == *x);
        }

        delete x.get();
    }

    SECTION("pointer works with arrays,")
    {
        auto x = openperf::memory::offset_ptr<long>(
            new long[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

        SECTION("can access items by index,")
        {
            for (int i = 0; i < 10; i++) { REQUIRE(x[i] == i); }
        }

        SECTION("can forward iterate by ++,")
        {
            int sum = 0;
            auto cursor = x[0];
            for (int i = 0; i < 10; i++) { sum += cursor++; }
            REQUIRE(sum == 45);
        }

        SECTION("can forward iterate by > 1,")
        {
            int sum = 0;
            auto cursor = x[0];
            for (int i = 0; i < 10; i += 2) {
                sum += cursor;
                cursor += 2;
            }
            REQUIRE(sum == 20);
        }

        SECTION("can backwards iterate by --")
        {
            int sum = 0;
            auto cursor = x[9];
            for (int i = 9; i >= 0; i--) { sum += cursor--; }
            REQUIRE(sum == 45);
        }

        SECTION("can backwards iterate by > 1,")
        {
            int sum = 0;
            auto cursor = x[9];
            for (int i = 9; i >= 0; i -= 2) {
                sum += cursor;
                cursor -= 2;
            }
            REQUIRE(sum == 25);
        }

        delete[] x.get();
    }

    SECTION("pointer works with standard library,")
    {
        auto x = openperf::memory::offset_ptr<int>(new int[100]);

        SECTION("can fill with iota,")
        {
            std::iota(x.get(), x.get() + 100, 1);

            SECTION("can accumulate values,")
            {
                REQUIRE(std::accumulate(x.get(), x.get() + 100, 0) == 5050);
            }
        }

        delete[] x.get();
    }

    SECTION("can use with structures,")
    {
        auto x = openperf::memory::offset_ptr<test_struct>(new test_struct());

        SECTION("can set values,")
        {
            static constexpr int value_foo = 1;
            static constexpr double value_bar = 1.0;
            static constexpr std::string_view value_baz = "test sting";

            x->foo = value_foo;
            x->bar = value_bar;
            x->baz = value_baz;

            SECTION("can read values,")
            {
                REQUIRE(x->foo == value_foo);
                REQUIRE(x->bar == value_bar);
                REQUIRE(x->baz == value_baz);
            }
        }

        delete x.get();
    }
}
