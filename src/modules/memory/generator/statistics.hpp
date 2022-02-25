#ifndef _OP_MEMORY_GENERATOR_STATISTICS_HPP_
#define _OP_MEMORY_GENERATOR_STATISTICS_HPP_

#include <algorithm>
#include <iostream>
#include <numeric>
#include <optional>

namespace openperf::memory::generator {

template <typename T> struct range
{
    T first;
    T last;

    range& operator+=(const range& other)
    {
        first = std::min(first, other.first);
        last = std::max(last, other.last);
        return (*this);
    }
};

template <typename T> range<T> operator+(range<T> lhs, const range<T>& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Clock> struct io_stats
{
    using duration = typename Clock::duration;

    size_t bytes_actual;
    size_t bytes_target;
    duration latency;
    size_t ops_actual;
    size_t ops_target;

    io_stats()
        : bytes_actual(0)
        , bytes_target(0)
        , latency(duration::zero())
        , ops_actual(0)
        , ops_target(0)
    {}

    io_stats& operator+=(const io_stats& other)
    {
        bytes_actual += other.bytes_actual;
        bytes_target += other.bytes_target;
        latency += other.latency;
        ops_actual += other.ops_actual;
        ops_target += other.ops_target;

        return (*this);
    }
};

template <typename Clock>
io_stats<Clock> operator+(io_stats<Clock> lhs, const io_stats<Clock>& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Clock> struct thread_stats
{
    using timestamp = typename Clock::time_point;
    using duration = typename Clock::duration;

    range<timestamp> time_;
    io_stats<Clock> read;
    io_stats<Clock> write;

    thread_stats()
        : time_({timestamp::max(), timestamp::min()})
    {}

    thread_stats(timestamp now)
        : time_({now, now})
    {}

    std::optional<timestamp> first() const
    {
        return (read.ops_actual + write.ops_actual > 0
                    ? std::optional<timestamp>{time_.first}
                    : std::nullopt);
    }

    std::optional<timestamp> last() const
    {
        return (read.ops_actual + write.ops_actual > 0
                    ? std::optional<timestamp>{time_.last}
                    : std::nullopt);
    }

    thread_stats& operator+=(const thread_stats& other)
    {
        time_ += other.time_;
        read += other.read;
        write += other.write;

        return (*this);
    }
};

template <typename Clock>
thread_stats<Clock> operator+(thread_stats<Clock> lhs,
                              const thread_stats<Clock>& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Clock>
inline thread_stats<Clock>
sum_stats(const std::vector<thread_stats<Clock>>& shards)
{
    auto sum = std::accumulate(std::begin(shards),
                               std::end(shards),
                               thread_stats<Clock>{},
                               std::plus<>{});

    return (sum);
}

template <typename Clock>
inline void dump(std::ostream& os, const thread_stats<Clock>& stats)
{
    os << "Memory Stats:" << std::endl;
    if (stats.read.ops_actual || stats.write.ops_actual) {
        os << " first: " << stats.first()->time_since_epoch().count()
           << std::endl;
        os << "  last: " << stats.last()->time_since_epoch().count()
           << std::endl;
    }

    os << " Read:" << std::endl;
    os << "  bytes (actual): " << stats.read.bytes_actual << std::endl;
    os << "  bytes (target): " << stats.read.bytes_target << std::endl;
    os << "  total latency: " << stats.read.latency.count() << std::endl;
    os << "  operations (actual): " << stats.read.ops_actual << std::endl;
    os << "  operations (target): " << stats.read.ops_target << std::endl;

    os << " Write:" << std::endl;
    os << "  bytes (actual): " << stats.write.bytes_actual << std::endl;
    os << "  bytes (target): " << stats.write.bytes_target << std::endl;
    os << "  total latency: " << stats.write.latency.count() << std::endl;
    os << "  operations (actual): " << stats.write.ops_actual << std::endl;
    os << "  operations (target): " << stats.write.ops_target << std::endl;
}

} // namespace openperf::memory::generator

#endif /* _OP_MEMORY_GENERATOR_STATISTICS_HPP_ */
