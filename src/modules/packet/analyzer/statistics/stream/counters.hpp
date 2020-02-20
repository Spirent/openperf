#ifndef _OP_ANALYZER_STATISTICS_STREAM_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_STREAM_COUNTERS_HPP_

#include <iostream>

#include "packet/analyzer/statistics/common.hpp"

namespace openperf::packet::analyzer::statistics::stream {

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
 * Summary statistics track min/max/total value.  We have two distinct
 * types:  one for tracking integral values and another for tracking
 * duration.
 * Note that each instance requires two types: one for the min/max value
 * and one for the total.  We typically use a smaller value for min/max
 * to conserve space.  We need a tuple for every received stream.
 */
template <typename...> struct summary;

template <typename ValueType, typename SumType>
struct summary<ValueType, SumType>
{
    using pop_t = ValueType;
    using sum_t = SumType;

    static_assert(std::is_arithmetic_v<pop_t>);
    static_assert(std::is_arithmetic_v<sum_t>);
    static_assert(std::is_convertible_v<pop_t, sum_t>);

    pop_t min = std::numeric_limits<pop_t>::max();
    pop_t max = std::numeric_limits<pop_t>::min();
    sum_t total = sum_t{0};
};

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct summary<std::chrono::duration<Rep1, Period1>,
               std::chrono::duration<Rep2, Period2>>
{
    using pop_t = std::chrono::duration<Rep1, Period1>;
    using sum_t = std::chrono::duration<Rep2, Period2>;
    static_assert(std::is_convertible_v<pop_t, sum_t>,
                  "Populate duration must be convertible to summary duration");

    pop_t min = pop_t::max();
    pop_t max = pop_t::min();
    sum_t total = sum_t::zero();
};

struct frame_length final : summary<uint16_t, int64_t>
{};

/*
 * Note: A signed 32 bit duration stores +/- 2 seconds.
 * A signed 64 bit duration stores +/- 262 years.  That's
 * totally overkill for our use, but there aren't any sizes
 * in between...
 */
using short_duration = std::chrono::duration<int32_t, std::nano>;
using long_duration = std::chrono::duration<int64_t, std::nano>;

struct interarrival final : summary<long_duration, long_duration>
{};

struct jitter_ipdv final : summary<short_duration, long_duration>
{};

struct jitter_rfc final : summary<short_duration, long_duration>
{};

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
inline void update(SummaryStat& stat,
                   typename SummaryStat::pop_t value) noexcept
{
    if (value < stat.min) { stat.min = value; }
    if (value > stat.max) { stat.max = value; }
    stat.total += value;
}

inline void update(latency& stat, typename latency::pop_t value) noexcept
{
    if (value < stat.min) { stat.min = value; }
    if (value > stat.max) { stat.max = value; }
    stat.total += value;
    stat.last_ = value;
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
            update(get_counter<interarrival, StatsTuple>(tuple), rx - *last_rx);
        }
    }

    if constexpr (has_type<frame_length, StatsTuple>::value) {
        update(get_counter<frame_length, StatsTuple>(tuple), length);
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
            update(latency_stats, delay);

            if (last_delay) {
                if constexpr (has_type<jitter_rfc, StatsTuple>::value) {
                    auto jitter = abs(delay - *last_delay);
                    update(get_counter<jitter_rfc, StatsTuple>(tuple), jitter);
                }

                if constexpr (has_type<jitter_ipdv, StatsTuple>::value) {
                    static_assert(has_type<sequencing, StatsTuple>::value);
                    if (get_counter<sequencing, StatsTuple>(tuple).run_length
                        > 1) {
                        /* frame is in sequence */
                        auto jitter = abs(delay - *last_delay);
                        update(get_counter<jitter_ipdv, StatsTuple>(tuple),
                               jitter);
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

} // namespace openperf::packet::analyzer::statistics::stream

#endif /* _OP_ANALYZER_STATISTICS_STREAM_COUNTERS_HPP_ */
