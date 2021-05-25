#ifndef _OP_TIMESYNC_CHRONO_HPP_
#define _OP_TIMESYNC_CHRONO_HPP_

#include <array>
#include <chrono>

#include "timesync/counter.hpp"
#include "utils/singleton.hpp"

namespace openperf::timesync::chrono {

struct timehands_data
{
    bintime offset;   /**< clock offset at reference tick value */
    counter::hz freq; /**< frequency to use for clock measurements */
    uint64_t scalar;  /**< conversion factor for ticks --> bintime fraction
                       * (1 << 64) / freq */
};

struct timehands
{
    counter::timecounter* counter; /**< counter to use for time measurements */
    timehands_data ref;            /**< reference data */
    timehands_data lerp;           /**< linear interpolation data */
    counter::ticks t_zero;         /**< reference tick value */
    counter::ticks t_lerp; /**< tick value where ref and lerp intersect */
    std::atomic<unsigned> generation; /**< structure version */
};

class keeper : public utils::singleton<keeper>
{
    static constexpr unsigned timehands_max = 8;
    static constexpr unsigned timehands_mask = timehands_max - 1;
    std::array<timehands, timehands_max> m_timehands;
    unsigned m_idx = 0;
    std::atomic<timehands*> m_now = nullptr;

public:
    void setup(counter::timecounter* tc);
    int sync(const bintime& timestamp, counter::ticks ticks, counter::hz freq);
    timehands* timehands_now() const;
};

inline std::pair<bintime, counter::ticks> monotime_ticks()
{
    using sec_t = decltype(bintime().bt_sec);

    auto tc = counter::timecounter_now.load(std::memory_order_relaxed);
    auto freq = tc->frequency().count();
    auto scalar = ((1ULL << 63) / freq) << 1;
    auto ticks = tc->now();

    auto now = bintime{.bt_sec = static_cast<sec_t>(ticks / freq),
                       .bt_frac = (ticks % freq) * scalar};

    return {now, ticks};
}

/*
 * Calculate the maximum clock error given the specified frequency error.
 * Error should be in Parts Per Billion (PPB).
 */
std::chrono::nanoseconds maximum_clock_error(uint64_t ppb_freq_error);

struct realtime
{
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<realtime, duration>;

    static constexpr bool is_steady = false;

    static time_point now() noexcept
    {
        using sec_t = decltype(bintime().bt_sec);

        timehands* th = nullptr;
        auto ticks = counter::ticks{0};
        auto gen = 0U;

        do {
            th = keeper::instance().timehands_now();
            gen = th->generation.load(std::memory_order_consume);
            ticks = th->counter->now();
        } while (gen == 0
                 || gen != th->generation.load(std::memory_order_consume));

        assert(th);

        auto delta = ticks - th->t_zero;
        const auto& data = (ticks < th->t_lerp ? th->lerp : th->ref);
        auto freq = data.freq.count();
        auto bt = bintime{.bt_sec = static_cast<sec_t>(delta / freq),
                          .bt_frac = (delta % freq) * data.scalar};

        return (time_point(to_duration(bt + data.offset)));
    }

    static std::time_t to_time_t(const time_point& tp) noexcept
    {
        return (std::time_t(std::chrono::duration_cast<std::chrono::seconds>(
                                tp.time_since_epoch())
                                .count()));
    }

    static time_point from_time_t(std::time_t t) noexcept
    {
        using from_t = std::chrono::time_point<realtime, std::chrono::seconds>;
        return (std::chrono::time_point_cast<realtime::duration>(
            from_t{std::chrono::seconds{t}}));
    }
};

struct monotime
{
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<monotime, duration>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept
    {
        using sec_t = decltype(bintime().bt_sec);

        auto tc = counter::timecounter_now.load(std::memory_order_relaxed);
        auto freq = tc->frequency().count();
        auto scalar = ((1ULL << 63) / freq) << 1;
        auto ticks = tc->now();

        auto now = bintime{.bt_sec = static_cast<sec_t>(ticks / freq),
                           .bt_frac = (ticks % freq) * scalar};

        return (time_point(to_duration(now)));
    }
};

} // namespace openperf::timesync::chrono

#endif /* _OP_TIMESYNC_CHRONO_HPP_ */
