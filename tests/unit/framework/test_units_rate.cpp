#include <chrono>
#include <iostream>

#include "catch.hpp"

#include "units/rate.h"
#include "units/data-rates.h"

TEST_CASE("rates", "[rate]")
{
    SECTION("constructors, ") {
        using rate = icp::units::rate<unsigned>;

        auto rate0 = rate{};
        REQUIRE(rate0.count() == 0);

        auto rate1 = rate{1};
        REQUIRE(rate1.count() == 1);

        auto rate2 = rate(2);
        REQUIRE(rate2.count() == 2);

        using megarate = icp::units::rate<unsigned, std::mega>;
        auto rate_mega = rate(megarate{1});
        REQUIRE(rate_mega.count() == std::mega::num);
    }

    SECTION("static, ") {
        using unsigned_rate = icp::units::rate<unsigned>;
        REQUIRE(unsigned_rate::zero().count() == 0);
        REQUIRE(unsigned_rate::min().count() == 0);
        REQUIRE(unsigned_rate::max().count() == std::numeric_limits<unsigned>::max());

        using signed_rate = icp::units::rate<int>;
        REQUIRE(signed_rate::zero().count() == 0);
        REQUIRE(signed_rate::min().count() == std::numeric_limits<int>::lowest());
        REQUIRE(signed_rate::max().count() == std::numeric_limits<int>::max());

        using double_rate = icp::units::rate<double>;
        REQUIRE(double_rate::zero().count() == 0);
        REQUIRE(double_rate::min().count() == std::numeric_limits<double>::lowest());
        REQUIRE(double_rate::max().count() == std::numeric_limits<double>::max());
    }

    SECTION("equality, ") {
        using per_second = icp::units::rate<uint64_t>;
        using per_minute = icp::units::rate<uint32_t, std::ratio<1, 60>>;
        using per_hour   = icp::units::rate<uint16_t, std::ratio<1, 3600>>;

        auto fast = per_second{100};
        auto med  = per_minute{10};
        auto slow = per_hour{1};

        REQUIRE(slow < fast);
        REQUIRE(slow < med);
        REQUIRE(med < fast);

        REQUIRE(fast > slow);
        REQUIRE(fast > med);
        REQUIRE(med > slow);

        REQUIRE(fast != med);
        REQUIRE(med != slow);
        REQUIRE(slow != fast);

        auto a = per_second{1};
        auto b = per_minute{60};
        auto c = per_hour{3600};

        REQUIRE(a == b);
        REQUIRE(b == c);
        REQUIRE(a == c);

        REQUIRE(a <= b);
        REQUIRE(b <= a);
        REQUIRE(b >= c);
        REQUIRE(c >= b);
    }

    SECTION("arithmetic, ") {
        using per_second = icp::units::rate<uint64_t>;

        auto one = per_second{1};
        auto two = per_second{2};
        auto five = per_second{5};

        REQUIRE(two == one + one);
        REQUIRE(two - one == one);
        REQUIRE(5 * one == five);

        /*
         * Note that div and module operations between rates return a scalar.
         * The units cancel out when dividing!
         */
        REQUIRE(five / 1 == five);
        REQUIRE(five / one == 5);
        REQUIRE(five % two == 1);
        REQUIRE(five % 2 == one);
    }

    SECTION("periods, ") {
        /* Suppose we have a 6 cylinder engine... */
        static constexpr unsigned num_cylinders = 6;

        /* engine speed is measured in rpm */
        using rpm = icp::units::rate<unsigned, std::ratio<1, 60>>;

        /* four stroke engine has 4 revolutions per cycle */
        static constexpr unsigned rev_per_cycle = 4;

        /* convenient time units */
        using ms = std::chrono::milliseconds;
        using us = std::chrono::microseconds;

        auto idle = rpm{800};
        auto rev_period = icp::units::get_period<ms>(idle);
        /* 60 / 800 = 0.075 */
        REQUIRE(rev_period.count() == 75);

        auto cycle_period = icp::units::get_period<ms>(idle / rev_per_cycle);
        /* (60 * 4) / 800 = 0.3 */
        REQUIRE(cycle_period.count() == 300);

        /* each spark plug fires once per revolution */
        auto spark_period = icp::units::get_period<us>(idle * num_cylinders);
        /* 60 / (800 * 6) = 0.0125 */
        REQUIRE(spark_period.count() == 12500);
    }

    SECTION("data rates, ") {
        using bps = icp::units::rate<uint64_t>;

        SECTION("byte, ") {
            using Bps = icp::units::rate<uint64_t, icp::units::bytes>;
            REQUIRE(Bps{1} == bps{8});
        }

        SECTION("kilo, ") {
            using kbps = icp::units::rate<uint64_t, icp::units::kilobits>;
            using Kibps = icp::units::rate<uint64_t, icp::units::kibibits>;
            using kBps = icp::units::rate<uint64_t, icp::units::kilobytes>;
            using KiBps = icp::units::rate<uint64_t, icp::units::kibibytes>;

            REQUIRE(kbps{1}  == bps{1000});
            REQUIRE(Kibps{1} == bps{1024});
            REQUIRE(kBps{1}  == bps{8000});
            REQUIRE(KiBps{1} == bps{8192});
        }

        SECTION("mega, ") {
            using Mbps = icp::units::rate<uint64_t, icp::units::megabits>;
            using Mibps = icp::units::rate<uint64_t, icp::units::mebibits>;
            using MBps = icp::units::rate<uint64_t, icp::units::megabytes>;
            using MiBps = icp::units::rate<uint64_t, icp::units::mebibytes>;

            REQUIRE(Mbps{1}  == bps{1000000});
            REQUIRE(Mibps{1} == bps{1048576});
            REQUIRE(MBps{1}  == bps{8000000});
            REQUIRE(MiBps{1} == bps{8388608});
        }

        SECTION("giga, ") {
            using Gbps = icp::units::rate<uint64_t, icp::units::gigabits>;
            using Gibps = icp::units::rate<uint64_t, icp::units::gibibits>;
            using GBps = icp::units::rate<uint64_t, icp::units::gigabytes>;
            using GiBps = icp::units::rate<uint64_t, icp::units::gibibytes>;

            REQUIRE(Gbps{1}  == bps{1000000000});
            REQUIRE(Gibps{1} == bps{1073741824});
            REQUIRE(GBps{1}  == bps{8000000000});
            REQUIRE(GiBps{1} == bps{8589934592});
        }

        SECTION("tera, ") {
            using Tbps = icp::units::rate<uint64_t, icp::units::terabits>;
            using Tibps = icp::units::rate<uint64_t, icp::units::tebibits>;
            using TBps = icp::units::rate<uint64_t, icp::units::terabytes>;
            using TiBps = icp::units::rate<uint64_t, icp::units::tebibytes>;

            REQUIRE(Tbps{1}  == bps{1000000000000});
            REQUIRE(Tibps{1} == bps{1099511627776});
            REQUIRE(TBps{1}  == bps{8000000000000});
            REQUIRE(TiBps{1} == bps{8796093022208});
        }
    }
}
