#include <cinttypes>

#include "core/op_log.h"
#include "timesync/chrono.hpp"

namespace openperf::timesync::chrono {

namespace detail {

static bintime time_at(const timehands& th, counter::ticks t)
{
    return (th.ref.ticks < t
                ? th.offset + to_bintime(t - th.ref.ticks, th.ref.freq)
                : th.offset - to_bintime(th.ref.ticks - t, th.ref.freq));
}

using namespace std::chrono_literals;
static constexpr auto jump_threshold = 5ms;

template <class ToRep, class Rep, class Period>
std::chrono::duration<ToRep> to_seconds(std::chrono::duration<Rep, Period> from)
{
    return (std::chrono::duration_cast<std::chrono::duration<ToRep>>(from));
}

/*
 * Return a value between [100, 1] ppm based on the amount we need to adjust.
 */
template <class Rep, class Period>
double get_clock_skew(std::chrono::duration<Rep, Period> d)
{
    static constexpr auto scalar = 1 / 60.0;
    return (d > d.zero() ? std::clamp((d * scalar).count(), 1e-6, 1e-4)
                         : std::clamp((d * scalar).count(), -1e-4, -1e-6));
}

} // namespace detail

void keeper::setup(counter::timecounter* tc)
{
    auto& th = m_timehands[0];
    th.counter = tc;
    th.ref.freq = tc->frequency();
    th.ref.scalar = ((1ULL << 63) / tc->frequency().count()) << 1;

    /* Guestimate an offset from the system clock */
    using clock = std::chrono::system_clock;
    auto ticks1 = tc->now();
    auto now = clock::now();
    auto ticks2 = tc->now();

    /* Naively assume now is halfway between ticks1 and ticks2 */
    th.ref.ticks = ticks1 + ((ticks2 - ticks1) >> 1);

    /* Convert the clock to a bintime value */
    th.offset = to_bintime(now.time_since_epoch());

    /* No lerping, yet */
    th.lerp.ticks = th.ref.ticks;
    th.generation.store(1, std::memory_order_release);
    m_now.store(std::addressof(th), std::memory_order_release);
    m_idx++;
}

int keeper::sync(const bintime& timestamp, counter::ticks t, counter::hz freq)
{
    timehands *th = nullptr, *th_next = nullptr;
    unsigned gen = 0, gen_next = 0;

    /*
     * XXX: I make no guarantees that the following operations permit an
     * atomic timehands update.
     */
    do {
        while (gen == 0) {
            th = m_now.load(std::memory_order_consume);
            gen = th->generation.load(std::memory_order_consume);
        }
        assert(th);
        th_next = std::addressof(m_timehands[m_idx & timehands_mask]);
        gen_next = th_next->generation.load(std::memory_order_consume);
        if (!th_next->generation.compare_exchange_weak(
                gen_next,
                0,
                std::memory_order_release,
                std::memory_order_acquire)) {
            return (EBUSY);
        }

        /*
         * Assumption at this point is that we've set the timehands generation
         * to 0 and are free to update the structure.
         */
        th_next->counter = th->counter;
        th_next->offset = timestamp;
        th_next->ref.ticks = t;
        th_next->ref.freq = freq;
        th_next->ref.scalar = ((1ULL << 63) / freq.count()) << 1;
        th_next->lerp = th_next->ref;

        /* the realtime of tick t using th_next should be the given timestamp */
        assert(detail::time_at(*th_next, t) == timestamp);

        /*
         * Figure out how to incorporate this new timestamp information.
         * If the value is below our jump threshold, calculate linear
         * interpolation data so we can slew the clock to the new time.
         */
        if (auto jump = to_duration(detail::time_at(*th, t) - timestamp);
            jump != decltype(jump)::zero()) {
            if (std::chrono::abs(jump) > detail::jump_threshold) {
                OP_LOG(OP_LOG_WARNING,
                       "Jumping clock by %.09f seconds.\n",
                       detail::to_seconds<double>(jump).count());
            } else {
                OP_LOG(OP_LOG_DEBUG,
                       "Slewing clock by %.09f seconds\n",
                       detail::to_seconds<double>(jump).count());
                th_next->lerp.freq = counter::hz{static_cast<uint64_t>(
                    freq.count() * (1 + detail::get_clock_skew(jump)))};
                th_next->lerp.scalar =
                    ((1ULL << 63) / th_next->lerp.freq.count()) << 1;
                th_next->lerp.ticks =
                    th_next->ref.ticks
                    + static_cast<counter::ticks>(
                        detail::to_seconds<double>(std::chrono::abs(jump))
                            .count()
                        * th_next->lerp.freq.count());
                assert(th_next->lerp.ticks >= th_next->ref.ticks);
            }
        }

        th_next->generation.store(gen_next + 1, std::memory_order_release);

    } while (!m_now.compare_exchange_weak(
        th, th_next, std::memory_order_release, std::memory_order_relaxed));

    m_idx++;
    return (0);
}

timehands* keeper::timehands_now() const
{
    return (m_now.load(std::memory_order_acquire));
}

} // namespace openperf::timesync::chrono
