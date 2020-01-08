#include "catch.hpp"

#include "timesync/history.hpp"

using namespace openperf::timesync;

TEST_CASE("history container", "[timesync]")
{
    auto h = history();
    auto f = counter::hz::zero();

    SECTION("can add timestamps, ")
    {

        REQUIRE(h.empty());
        REQUIRE(h.size() == 0);

        auto a =
            timestamp{.Ta = 100100, .Tb = {1, 0}, .Te = {1, 1}, .Tf = 100200};

        h.insert(a, f);

        REQUIRE(!h.empty());
        REQUIRE(h.size() == 1);

        auto b =
            timestamp{.Ta = 200100, .Tb = {2, 0}, .Te = {2, 1}, .Tf = 200200};

        h.insert(b, f);

        REQUIRE(!h.empty());
        REQUIRE(h.size() == 2);

        /*
         * timestamp b - timestamp a == 1 second;
         * hence the duration should match.
         */
        auto dur = h.duration();
        REQUIRE(dur.bt_sec == 1);
        REQUIRE(dur.bt_frac == 0);
    }

    SECTION("can apply function to timestamps, ")
    {
        REQUIRE(h.empty());

        /* Generate some timestamps */
        static constexpr auto ntp_ts_delta = 1UL << 32;
        static constexpr int nb_timestamps = 100;

        for (auto i = 0; i < nb_timestamps; i++) {
            auto ts = timestamp{.Ta = 1000UL * i,
                                .Tb = {i, 0},
                                .Te = {i, ntp_ts_delta},
                                .Tf = 1000UL * i + 1000};
            h.insert(ts, f);
        }

        /* Make sure we can visit all timestamps */
        int ts_count1 = 0;
        auto range1 =
            std::make_pair(h.lower_bound(0), h.upper_bound(nb_timestamps));
        h.apply(
            range1,
            [](const timestamp&, counter::hz, int& count) { count++; },
            ts_count1);
        REQUIRE(ts_count1 == nb_timestamps);

        /* Now, retry with an invalid range */
        int ts_count2 = 0;
        auto range2 = std::make_pair(h.lower_bound(nb_timestamps),
                                     h.upper_bound(2 * nb_timestamps));
        h.apply(
            range2,
            [](const timestamp&, counter::hz, int& count) { count++; },
            ts_count2);
        REQUIRE(ts_count2 == 0);

        /* Retry valid range after clearing our data; count should be 0 */
        h.clear();
        int ts_count3 = 0;
        auto range3 =
            std::make_pair(h.lower_bound(0), h.upper_bound(nb_timestamps));
        h.apply(
            range3,
            [](const timestamp&, counter::hz, int& count) { count++; },
            ts_count3);
        REQUIRE(ts_count3 == 0);
    }

    SECTION("can find inserted timestamps, ")
    {
        REQUIRE(h.empty());

        auto a =
            timestamp{.Ta = 100100, .Tb = {1, 0}, .Te = {1, 1}, .Tf = 100200};

        REQUIRE(!h.contains(a));
        h.insert(a, f);
        REQUIRE(h.contains(a));

        auto b =
            timestamp{.Ta = 200100, .Tb = {2, 0}, .Te = {2, 1}, .Tf = 200200};

        REQUIRE(!h.contains(b));
        h.insert(b, f);
        REQUIRE(h.contains(b));
    }
}
