#include "benchmark.h"

namespace pga::benchmark {

using ticks = std::chrono::time_point<clock>;

static constexpr unsigned timer_calc_iterations = 100;

struct tau {
    clock::duration accuracy;
    clock::duration precision;
};

static tau calculate_tau(unsigned iterations)
{
    std::vector<ticks> timestamps;
    timestamps.reserve(iterations);
    std::generate_n(std::begin(timestamps),
                    iterations,
                    [](){ return (clock::now()); });


    /*
     * std::adjacent_differnce seems like an obvious choice here, but it
     * can't handle the type change between tick and duration :(
     */
    std::vector<clock::duration> intervals;
    intervals.reserve(iterations - 1);
    for (unsigned i = 0; i < iterations - 1; i++) {
        intervals.emplace_back(timestamps[i + 1] - timestamps[i]);
    }

    return (tau{ .accuracy = *std::max_element(begin(intervals),
                                               end(intervals)),
                 .precision = clock::duration(1) });
}

static unsigned calculate_benchmark_iterations(unsigned timer_iterations)
{
    auto tau = calculate_tau(timer_iterations);
    return (tau.accuracy / tau.precision);
}

unsigned iterations()
{
    static auto iterations_ = calculate_benchmark_iterations(timer_calc_iterations);
    return (iterations_);
}

}
