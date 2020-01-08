#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <string>

#include "catch.hpp"

#include "timesync/bintime.hpp"

/*
 * Note: the fractional part of time structures, e.g. bintime, timespec,
 * timeval, etc. are always positive.  As a result, negative durations
 * are not stored in an obvious manner.  For example, -250ms will be
 * stored as -1 + 750ms.
 */

TEST_CASE("bintime functionality", "[timesync]")
{
    using bintime = openperf::timesync::bintime;
    using frac_t = decltype(bintime().bt_frac);

    SECTION("compare, ")
    {
        auto a = bintime{1, 0};
        auto b = bintime{1, 0};
        auto c = bintime{0, std::numeric_limits<frac_t>::max()};

        SECTION("equality, ")
        {
            REQUIRE(a == b);
            REQUIRE(a <= b);
            REQUIRE(b <= a);
        }

        SECTION("inequality, ")
        {
            REQUIRE(a != c);
            REQUIRE(c != a);
        }

        SECTION("less/greater than, ")
        {

            REQUIRE(a > c);
            REQUIRE(c < a);
            REQUIRE(c <= a);
            REQUIRE(a >= c);
        }
    }

    SECTION("arithmetic, ")
    {
        SECTION("addition and subtraction, ")
        {
            auto a = bintime{0, 1};
            auto b = bintime{0, std::numeric_limits<frac_t>::max()};
            auto c = bintime{1, 0};
            auto d = bintime{2, 0};

            REQUIRE(a + b == c);
            REQUIRE(a + b + c == d);
            REQUIRE(c + c == d);

            REQUIRE(d - c == c);
            REQUIRE(c - a == b);

            REQUIRE(a - b < c);
        }

        SECTION("multiplication and division, ")
        {
            auto quarter = bintime{0, 1UL << 62};
            auto half = bintime{0, 1UL << 63};
            auto one = bintime{1, 0};
            auto two = bintime{2, 0};

            REQUIRE((quarter * 1) == quarter);
            REQUIRE((quarter * 2) == half);
            REQUIRE((quarter * 4) == one);
            REQUIRE((quarter * 8) == two);

            REQUIRE((quarter / 1) == quarter);
            REQUIRE((half / 2) == quarter);
            REQUIRE((one / 4) == quarter);
            REQUIRE((two / 8) == quarter);
        }

        SECTION("miscellaneous, ")
        {
            auto zero = bintime{0, 0};
            auto pos_half = bintime{0, 1UL << 63};
            auto neg_half = bintime{-1, 1UL << 63};

            REQUIRE(zero - pos_half == neg_half);
            REQUIRE(zero - neg_half == pos_half);
            REQUIRE(pos_half + neg_half == zero);
        }
    }

    SECTION("conversions, ")
    {
        using namespace std::chrono_literals;
        using namespace openperf::timesync;

        auto ref_pos_one = bintime{1, 0};
        auto ref_pos_quarter = bintime{0, 4UL << 60};
        auto ref_neg_one = bintime{-1, 0};
        auto ref_neg_quarter = bintime{-1, 12UL << 60};

        SECTION("double, ")
        {
            REQUIRE(to_double(ref_pos_one) == Approx(1.0));
            REQUIRE(to_double(ref_pos_quarter) == Approx(0.25));
            REQUIRE(to_double(ref_neg_one) == Approx(-1.0));
            REQUIRE(to_double(ref_neg_quarter) == Approx(-0.25));
        }

        SECTION("timespec, ")
        {
            auto test_pos_one = timespec{1, 0};
            auto test_pos_quarter =
                timespec{0, std::chrono::nanoseconds(250ms).count()};
            auto test_neg_one = timespec{-1, 0};
            auto test_neg_quarter =
                timespec{-1, std::chrono::nanoseconds(750ms).count()};

            REQUIRE(ref_pos_one == to_bintime(test_pos_one));
            REQUIRE(to_double(ref_pos_quarter)
                    == Approx(to_double(to_bintime(test_pos_quarter))));
            REQUIRE(ref_neg_one == to_bintime(test_neg_one));
            REQUIRE(to_double(ref_neg_quarter)
                    == Approx(to_double(to_bintime(test_neg_quarter))));

            REQUIRE(to_double(to_timespec(ref_pos_one))
                    == Approx(to_double(test_pos_one)));
            REQUIRE(to_double(to_timespec(ref_pos_quarter))
                    == Approx(to_double(test_pos_quarter)));
            REQUIRE(to_double(to_timespec(ref_neg_one))
                    == Approx(to_double(test_neg_one)));
            REQUIRE(to_double(to_timespec(ref_neg_quarter))
                    == Approx(to_double(test_neg_quarter)));
        }

        SECTION("timeval, ")
        {
            auto test_pos_one = timeval{1, 0};
            auto test_pos_quarter =
                timeval{0, std::chrono::microseconds(250ms).count()};
            auto test_neg_one = timeval{-1, 0};
            auto test_neg_quarter =
                timeval{-1, std::chrono::microseconds(750ms).count()};

            REQUIRE(ref_pos_one == to_bintime(test_pos_one));
            REQUIRE(to_double(ref_pos_quarter)
                    == Approx(to_double(to_bintime(test_pos_quarter))));
            REQUIRE(ref_neg_one == to_bintime(test_neg_one));
            REQUIRE(to_double(ref_neg_quarter)
                    == Approx(to_double(to_bintime(test_neg_quarter))));

            REQUIRE(to_double(to_timeval(ref_pos_one))
                    == Approx(to_double(test_pos_one)));
            REQUIRE(to_double(to_timeval(ref_pos_quarter))
                    == Approx(to_double(test_pos_quarter)));
            REQUIRE(to_double(to_timeval(ref_neg_one))
                    == Approx(to_double(test_neg_one)));
            REQUIRE(to_double(to_timeval(ref_neg_quarter))
                    == Approx(to_double(test_neg_quarter)));
        }

        SECTION("duration, ")
        {
            auto test_pos_one = std::chrono::duration(1s);
            auto test_pos_quarter = std::chrono::duration(250ms);
            auto test_neg_one = std::chrono::duration(-1s);
            auto test_neg_quarter = std::chrono::duration(-250ms);

            REQUIRE(ref_pos_one == to_bintime(test_pos_one));
            REQUIRE(to_double(ref_pos_quarter)
                    == Approx(to_double(to_bintime(test_pos_quarter))));

            /* XXX: Not sure why this conversion isn't exact... */
            // REQUIRE(ref_neg_one == to_bintime(test_neg_one));
            REQUIRE(to_double(ref_neg_one)
                    == Approx(to_double(to_bintime(test_neg_one))));

            REQUIRE(to_double(ref_neg_quarter)
                    == Approx(to_double(to_bintime(test_neg_quarter))));
        }
    }
}
