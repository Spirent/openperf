#include <chrono>
#include <thread>

#include "tl/expected.hpp"
#include "timesync/counter.hpp"

namespace openperf::timesync::counter {

static inline void lfence() { __asm __volatile("lfence" : : : "memory"); }

static inline uint64_t rdtsc()
{
    uint32_t low, high;
    __asm __volatile("rdtsc" : "=a"(low), "=d"(high));
    return (static_cast<uint64_t>(high) << 32 | low);
}

static inline uint64_t rdtsc_lfence()
{
    lfence();
    return (rdtsc());
}

/* Just a quick and dirty means to get a frequency approximation */
static hz measure_tsc_frequency()
{
    using namespace std::chrono_literals;
    using clock = std::chrono::steady_clock;

    auto clk_start = clock::now();
    auto tsc_start = rdtsc_lfence();
    std::this_thread::sleep_for(10ms);
    auto clk_ticks = clock::now() - clk_start;
    auto tsc_ticks = rdtsc_lfence() - tsc_start;

    return (hz{tsc_ticks * clock::duration{1s}.count() / clk_ticks.count()});
}

class tsc final : public timecounter::registrar<tsc>
{
public:
    tsc()
        : m_freq(measure_tsc_frequency())
    {}

    std::string_view name() const override { return "TSC"; }

    ticks now() const override { return (rdtsc_lfence()); }

    hz frequency() const override { return (m_freq); }

    static constexpr int priority = 10;

    int static_priority() const override { return (priority); }

private:
    hz m_freq;
};

} // namespace openperf::timesync::counter
