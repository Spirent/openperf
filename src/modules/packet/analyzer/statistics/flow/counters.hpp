#ifndef _OP_ANALYZER_STATISTICS_FLOW_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_COUNTERS_HPP_

#include <iostream>

#include "packet/analyzer/statistics/common.hpp"

namespace openperf::packet::analyzer::statistics::flow {

struct sequencing
{
    stat_t dropped = 0;
    stat_t duplicate = 0;
    stat_t late = 0;
    stat_t reordered = 0;

    stat_t in_order = 0;
    stat_t run_length = 0;

    std::optional<uint32_t> last_seq = std::nullopt;
};

template <typename T> struct is_duration : std::false_type
{};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{};

/**
 * Summary statistics track min/max/total value and variance.  We have two
 * distinct versions of this structure: one for tracking arithmetic values
 * and one for tracking durations.
 * Note that each instance requires two types: one for the min/max value
 * and one for the total.  We typically use a smaller value for min/max
 * to conserve space.  We need a tuple for every received flow, so conserving
 * space here has scalability benefits.
 * A third type, based on float, is used to track variance and average.
 * We prefer float over double for a number of reasons:
 * - floats are smaller
 * - floats are faster on the system I benchmarked (though both floats and
 *   doubles are both faster that 64 bit integers!)
 * - we don't need the extra resolution
 */
template <typename...> struct summary;

template <typename ValueType, typename SumType>
struct summary<ValueType, SumType>
{
    using pop_t = ValueType;
    using sum_t = SumType;
    using var_t = float;

    static_assert(std::is_arithmetic_v<pop_t>, "ValueType must be arithemtic");
    static_assert(std::is_arithmetic_v<sum_t>, "SumType must be arithmetic");
    static_assert(std::is_convertible_v<pop_t, var_t>);

    pop_t min = std::numeric_limits<pop_t>::max();
    pop_t max = std::numeric_limits<pop_t>::min();
    sum_t total = sum_t{0};
    var_t m2 = var_t{0};
};

template <typename Rep1, typename Rep2, typename Period1, typename Period2>
struct summary<std::chrono::duration<Rep1, Period1>,
               std::chrono::duration<Rep2, Period2>>
{
    using pop_t = std::chrono::duration<Rep1, Period1>;
    using sum_t = std::chrono::duration<Rep2, Period2>;
    using var_t = std::chrono::duration<float, Period1>;

    static_assert(
        std::is_convertible_v<pop_t, sum_t>,
        "Population duration must be convertible to summary duration");
    static_assert(std::is_convertible_v<Rep1, float>,
                  "Population representation must be convertible to float");

    pop_t min = pop_t::max();
    pop_t max = pop_t::min();
    sum_t total = sum_t::zero();
    var_t m2 = var_t::zero();
};

struct frame_length final : summary<uint16_t, int64_t>
{};

/*
 * Note: A signed 32 bit duration stores +/- 2 seconds.
 * A signed 64 bit duration stores +/- 262 years.  That's
 * totally overkill for our use, but there aren't any sizes
 * in between...
 */
using long_duration = std::chrono::duration<int64_t, std::nano>;
using short_duration = std::chrono::duration<int32_t, std::nano>;
using ushort_duration = std::chrono::duration<uint32_t, std::nano>;

struct interarrival final : summary<long_duration, long_duration>
{};

/*
 * In order to fit all possible values, jitter_ipdv needs to have twice the
 * range of latency.  It must be able to store the difference between
 * latency::max() and latency::min(). To do that with a signed, 32-bit value
 * we use 2ns as our unit.
 * This version of jitter is defined in RFC 3393.
 */
using jitter_duration =
    std::chrono::duration<int32_t,
                          std::ratio_multiply<std::ratio<2>, std::nano>>;

struct jitter_ipdv final : summary<jitter_duration, long_duration>
{};

/*
 * Jitter RFC uses an absolute value, so we can simply use an unsigned
 * population value to cover the range.
 * This version of jitter is defined in RFC 4689
 */
struct jitter_rfc final : summary<ushort_duration, long_duration>
{};

/*
 * Latency is more properly called delay.  It is also known as one-way delay.
 * It is calculated by subtracting the receive time from the transmit time.
 */
struct latency final : summary<short_duration, long_duration>
{
    long_duration last_ = long_duration::zero();

    std::optional<long_duration> last() const
    {
        return (total.count() ? std::optional<long_duration>{last_}
                              : std::nullopt);
    }
};

/**
 * Update functions for stat structures
 **/

inline void
update(sequencing& stat, uint32_t seq_num, uint32_t late_threshold) noexcept
{
    /*
     * Since sequence numbers are unsigned, we have to perform modular
     * arithmetic on sequence numbers.  If any of our operations exceed
     * this value, then we know we've overflowed the counter.
     */
    static constexpr auto max_sequence = std::numeric_limits<uint32_t>::max();

    /* Note: expected sequence number == last_seq + 1 */
    if (!stat.last_seq || stat.last_seq.value() == seq_num - 1) {
        /* Sequence number is what we expect */
        stat.in_order++;
        stat.run_length++;
        stat.last_seq = seq_num;
    } else {
        /* delta = rx - expected; note this might overflow */
        auto delta = seq_num - stat.last_seq.value() - 1;
        if (delta < max_sequence / 2) {
            /* Sequence number is from the future! */
            stat.run_length = 1;
            stat.dropped += delta;
            stat.in_order++;
            stat.last_seq = seq_num;
        } else {
            /* Sequence number is in the past */
            if (delta > max_sequence - stat.run_length) {
                /* Within the run window, so we've seen it before */
                stat.duplicate++;
            } else {
                if (stat.dropped) { stat.dropped--; }
                if (delta > max_sequence - late_threshold) {
                    /* Within the late sequence window */
                    stat.reordered++;
                } else {
                    /* Where you been, buddy? */
                    stat.late++;
                }
            }
        }
    }
}

template <typename SummaryStat>
inline std::enable_if_t<std::is_arithmetic_v<typename SummaryStat::pop_t>, void>
update(SummaryStat& stat,
       typename SummaryStat::pop_t value,
       statistics::stat_t count) noexcept
{
    if (value < stat.min) { stat.min = value; }
    if (value > stat.max) { stat.max = value; }
    stat.total += value;

    if (stat.min != stat.max) { /* iff count > 1 */
        /*
         * The variance calculation below is based on Welford's online algorithm
         * to generate variance:
         * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
         * However, we've performed some algebra on the standard implementation
         * in order to minimize the number of CPU instructions.
         *
         * Let value = v, total = t, and count = c.
         * Given m2_next = m2 + (v - mean_prev) * (v - mean_curr)
         *               = m2 + (v - (t - v) / (c - 1)) * (v - t / c)
         *               = m2 + ((cv - v - t + v) / (c - 1)) * ((cv - t) / c)
         *               = m2 + ((cv - t)^2) / (c * (c - 1))
         *
         * Note: casting `count` to a float should eliminate almost any
         * reasonable potential for multiplication overflow, since FLT_MAX ~
         * 2^128.
         */
        const auto c = static_cast<float>(count);
        auto tmp = (c * value) - stat.total;
        auto num = tmp * tmp;
        auto den = c * (c - 1);
        stat.m2 += num / den;
    }
}

template <typename SummaryStat>
inline std::enable_if_t<is_duration<typename SummaryStat::pop_t>::value, void>
update(SummaryStat& stat,
       typename SummaryStat::pop_t value,
       statistics::stat_t count) noexcept
{
    if (value < stat.min) { stat.min = value; }
    if (value > stat.max) { stat.max = value; }
    stat.total += value;

    if (stat.min != stat.max) { /* iff count > 1 */
        /* See algorithm comment above */
        const auto c = static_cast<float>(count);
        auto tmp = (c * value) - stat.total;
        auto num = tmp * tmp.count();
        auto den = c * (c - 1);
        stat.m2 += num / den;
    }
}

inline void update(latency& stat,
                   typename latency::pop_t value,
                   statistics::stat_t count) noexcept
{
    stat.last_ = value;
    update<latency>(stat, value, count);
}

template <typename SumType, typename VarianceType>
std::enable_if_t<std::is_arithmetic_v<VarianceType>, VarianceType>
add_variance(statistics::stat_t x_count,
             SumType x_total,
             VarianceType x_m2,
             statistics::stat_t y_count,
             SumType y_total,
             VarianceType y_m2)
{
    if (x_count + y_count == 0) { return VarianceType{0}; }

    const auto x_avg = x_count ? static_cast<VarianceType>(x_total) / x_count
                               : VarianceType{0};
    const auto y_avg = y_count ? static_cast<VarianceType>(y_total) / y_count
                               : VarianceType{0};
    const auto delta = x_avg - y_avg;
    return (x_m2 + y_m2
            + ((delta * delta) * x_count * y_count / (x_count + y_count)));
}

template <typename SumType, typename VarianceType>
std::enable_if_t<is_duration<VarianceType>::value, VarianceType>
add_variance(statistics::stat_t x_count,
             SumType x_total,
             VarianceType x_m2,
             statistics::stat_t y_count,
             SumType y_total,
             VarianceType y_m2)
{
    if (x_count + y_count == 0) { return VarianceType::zero(); }

    const auto x_avg =
        x_count ? std::chrono::duration_cast<VarianceType>(x_total) / x_count
                : VarianceType::zero();
    const auto y_avg =
        y_count ? std::chrono::duration_cast<VarianceType>(y_total) / y_count
                : VarianceType::zero();
    const auto delta = x_avg - y_avg;
    return (
        x_m2 + y_m2
        + ((delta * delta.count()) * x_count * y_count / (x_count + y_count)));
}

/**
 * Update function for arbitrary collection of statistics in a tuple.
 **/

template <typename StatsTuple>
void update(StatsTuple& tuple,
            uint16_t length,
            counter::timestamp rx,
            std::optional<counter::timestamp> tx,
            std::optional<uint32_t> seq_num)
{
    static_assert(has_type<counter, StatsTuple>::value);

    auto& frames = get_counter<counter, StatsTuple>(tuple);
    auto last_rx = frames.last();
    update(frames, 1, rx);
    if constexpr (has_type<interarrival, StatsTuple>::value) {
        if (last_rx) {
            update(get_counter<interarrival, StatsTuple>(tuple),
                   rx - *last_rx,
                   frames.count - 1);
        }
    }

    if constexpr (has_type<frame_length, StatsTuple>::value) {
        update(
            get_counter<frame_length, StatsTuple>(tuple), length, frames.count);
    }

    if constexpr (has_type<sequencing, StatsTuple>::value) {
        if (seq_num) {
            update(get_counter<sequencing, StatsTuple>(tuple), *seq_num, 1000);
        }
    }

    if constexpr (has_type<latency, StatsTuple>::value) {
        if (tx) {
            auto& latency_stats = get_counter<latency, StatsTuple>(tuple);
            auto last_delay = latency_stats.last();
            auto delay = rx - *tx;
            update(latency_stats, delay, frames.count);

            if (last_delay) {
                if constexpr (has_type<jitter_rfc, StatsTuple>::value) {
                    auto jitter = std::chrono::abs(delay - *last_delay);
                    update(get_counter<jitter_rfc, StatsTuple>(tuple),
                           jitter,
                           frames.count);
                }

                if constexpr (has_type<jitter_ipdv, StatsTuple>::value) {
                    static_assert(has_type<sequencing, StatsTuple>::value);
                    if (get_counter<sequencing, StatsTuple>(tuple).run_length
                        > 1) {
                        /* frame is in sequence */
                        auto jitter =
                            std::chrono::duration_cast<jitter_ipdv::pop_t>(
                                delay - *last_delay);
                        update(get_counter<jitter_ipdv, StatsTuple>(tuple),
                               jitter,
                               frames.count);
                    }
                }
            }
        }
    }
}

/**
 * Debug methods
 **/

void dump(std::ostream& os, const sequencing& stat);
void dump(std::ostream& os, const frame_length& stat);
void dump(std::ostream& os, const interarrival& stat);
void dump(std::ostream& os, const jitter_ipdv& stat);
void dump(std::ostream& os, const jitter_rfc& stat);
void dump(std::ostream& os, const latency& stat);

template <typename StatsTuple>
void dump(std::ostream& os, const StatsTuple& tuple)
{
    os << __PRETTY_FUNCTION__ << std::endl;

    if constexpr (has_type<counter, StatsTuple>::value) {
        dump(os, get_counter<counter, StatsTuple>(tuple), "Frames");
    }

    if constexpr (has_type<sequencing, StatsTuple>::value) {
        dump(os, get_counter<sequencing, StatsTuple>(tuple));
    }

    if constexpr (has_type<frame_length, StatsTuple>::value) {
        dump(os, get_counter<frame_length, StatsTuple>(tuple));
    }

    if constexpr (has_type<interarrival, StatsTuple>::value) {
        dump(os, get_counter<interarrival, StatsTuple>(tuple));
    }

    if constexpr (has_type<jitter_ipdv, StatsTuple>::value) {
        dump(os, get_counter<jitter_ipdv, StatsTuple>(tuple));
    }

    if constexpr (has_type<jitter_rfc, StatsTuple>::value) {
        dump(os, get_counter<jitter_rfc, StatsTuple>(tuple));
    }

    if constexpr (has_type<latency, StatsTuple>::value) {
        dump(os, get_counter<latency, StatsTuple>(tuple));
    }
}

/*
 * Enforce hard limit on the expected worst case tuple size.
 * This is totally a self imposed limit, but let's now blow
 * past it without a good reason to do so.
 */
static_assert(sizeof(std::tuple<counter,
                                frame_length,
                                interarrival,
                                sequencing,
                                latency,
                                jitter_rfc>)
                  <= 192,
              "Flow count structures are too large!");

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_COUNTERS_HPP_ */
