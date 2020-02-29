#include <cinttypes>

#include "core/op_log.h"
#include "timesync/chrono.hpp"

namespace openperf::timesync::chrono {

namespace detail {

static bintime time_at(const timehands& th, counter::ticks t)
{
    const auto& data = (t < th.t_lerp ? th.lerp : th.ref);
    return (th.t_zero < t ? data.offset + to_bintime(t - th.t_zero, data.freq)
                          : data.offset - to_bintime(th.t_zero - t, data.freq));
}

using namespace std::chrono_literals;
static constexpr auto jump_threshold = 5ms;

template <class ToRep, class Rep, class Period>
std::chrono::duration<ToRep> to_seconds(std::chrono::duration<Rep, Period> from)
{
    return (std::chrono::duration_cast<std::chrono::duration<ToRep>>(from));
}

/*
 * Return a value between [200, 2] ppm based on the amount we need to adjust.
 * The lower bound on the slew is to ensure that the clock converges in a
 * finite amount of time. The longer we take to converge with the reference
 * time, the farther it will drift!
 */
template <class Rep, class Period>
double get_clock_slew(std::chrono::duration<Rep, Period> d)
{
    /*
     * Arbitrary scalar value.  The trick is to find a slew slow enough that
     * the clock doesn't oscillate noticeably, but not so slow that the
     * reference clock drifts away before we can catch it.
     */
    static constexpr auto scalar = 1 / 20.0;
    auto scaled = to_seconds<double>(d) * scalar;
    return (d > d.zero() ? std::clamp(scaled.count(), 2e-6, 2e-4)
                         : std::clamp(scaled.count(), -2e-4, -2e-6));
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
    th.t_zero = ticks1 + ((ticks2 - ticks1) >> 1);

    /* Convert the clock to a bintime value */
    th.ref.offset = to_bintime(now.time_since_epoch());

    /* No lerping, yet */
    th.t_lerp = th.t_zero;
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
        th_next->ref.offset = timestamp;
        th_next->t_zero = t;
        th_next->t_lerp = t; /* might not need to lerp */
        th_next->ref.freq = freq;
        th_next->ref.scalar = ((1ULL << 63) / freq.count()) << 1;
        th_next->lerp = th_next->ref;

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
                auto slew = detail::get_clock_slew(jump);
                OP_LOG(OP_LOG_DEBUG,
                       "Slewing clock by %.09f seconds at %d ppm\n",
                       detail::to_seconds<double>(jump).count(),
                       static_cast<int>(slew * 1000000));
                th_next->lerp.offset = detail::time_at(*th, t);
                th_next->lerp.freq = counter::hz{
                    static_cast<uint64_t>(freq.count() * (1 + slew))};
                th_next->lerp.scalar =
                    ((1ULL << 63) / th_next->lerp.freq.count()) << 1;

                /* Figure out how long it will take to make up the jump */
                auto slew_ticks = static_cast<counter::ticks>(
                    th_next->lerp.freq.count()
                    * detail::to_seconds<double>(jump).count() / slew);
                th_next->t_lerp = th_next->t_zero + slew_ticks;

                /* Slewed time should be continuous */
                assert(
                    std::chrono::abs(to_duration(
                        detail::time_at(*th, t) - detail::time_at(*th_next, t)))
                    <= std::chrono::nanoseconds{1});
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
