#include <cassert>
#include <cinttypes>
#include <fstream>
#include <sstream>
#include <string>

#include "catch.hpp"

#include "timesync/clock.hpp"

using namespace openperf::timesync;
using namespace std::chrono_literals;

namespace openperf::timesync::counter {
std::atomic<timecounter*> timecounter_now = nullptr;
}

static constexpr std::string_view clock_data = "data/clock_test_data.csv";

using timecounter_vector = std::vector<std::unique_ptr<counter::timecounter>>;
static timecounter_vector initialize_timecounters()
{
    timecounter_vector timecounters;
    counter::timecounter::make_all(timecounters);
    REQUIRE(!timecounters.empty());
    counter::timecounter_now.store(timecounters.front().get(),
                                   std::memory_order_release);

    return (timecounters);
}

/* Filename should contain a comma separated list of timestamps */
static std::vector<timestamp> load_timestamps(std::string_view filename)
{
    auto timestamps = std::vector<timestamp>{};

    auto f = std::fstream(filename.data());
    std::string line;
    while (std::getline(f, line, '\n')) {
        auto stream = std::stringstream(line);
        uint64_t Ta = 0, Tf = 0;
        double Tb = 0, Te = 0;

        stream >> Ta;
        assert(stream.peek() == ',');
        stream.ignore();

        stream >> Tb;
        assert(stream.peek() == ',');
        stream.ignore();

        stream >> Te;
        assert(stream.peek() == ',');
        stream.ignore();

        stream >> Tf;

        timestamps.push_back(timestamp{Ta, to_bintime(Tb), to_bintime(Te), Tf});
    }

    return (timestamps);
}

static int
do_update(const bintime& time, counter::ticks ticks, counter::hz freq)
{
    static constexpr auto max_jump = 1ms;
    static bintime last_time = {};
    static counter::ticks last_ticks = {};
    static counter::hz last_freq = {};
    static auto last_scalar = 0UL;
    static auto nb_updates = 0;

    /*
     * Skip the first proper update as the clock may make a large
     * initial jump.
     */
    if (nb_updates++ > 1) {
        int64_t d_ticks = static_cast<int64_t>(ticks) - last_ticks;
        auto d_time = bintime{static_cast<time_t>(d_ticks / last_freq.count()),
                              (d_ticks % last_freq.count()) * last_scalar};
        auto now_calculated =
            (d_ticks > 0 ? last_time + d_time : last_time - d_time);

        auto jump = to_duration(now_calculated - time);

        /*
         * Sanity check of jump values.  Since we know our input is from a good
         * server, this shouldn't be exceeded.
         */
        REQUIRE(std::chrono::abs(jump) < max_jump);
    }

    last_time = time;
    last_ticks = ticks;
    last_freq = freq;
    last_scalar = ((1ULL << 63) / freq.count()) << 1;

    return (0);
}

TEST_CASE("clock sync", "[timesync]")
{
    /* Initialize and setup timecounter */
    auto timecounters = initialize_timecounters();
    auto clock = openperf::timesync::clock(do_update);

    SECTION("load timestamps, ")
    {
        auto timestamps = load_timestamps(clock_data);
        REQUIRE(!timestamps.empty());

        SECTION("update clock, ")
        {
            std::for_each(std::begin(timestamps),
                          std::end(timestamps),
                          [&clock](const auto& ts) {
                              clock.update(ts.Ta, ts.Tb, ts.Te, ts.Tf);
                          });

            /*
             * XXX: because our test data comes from a replay of past
             * timesync data, we can't actually verify that the clock
             * is synced.  There is no real correlation between test
             * data timestamps and current counter timestamps.
             */
            // REQUIRE(clock.synced());
        }
    }
}
