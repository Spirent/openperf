#ifndef _OP_MEMORY_STAT_
#define _OP_MEMORY_STAT_

#include <cinttypes>
#include <chrono>
#include <optional>
#include <functional>

namespace openperf::memory::internal {

using namespace std::chrono_literals;

template<class T>
std::optional<T> bind_binary(std::function<T (T, T)> f, std::optional<T> v1, std::optional<T> v2)
{
    if (v1.has_value() && v2.has_value())
        return f(v1.value(), v2.value());

    return (v1.has_value()) ? v1 : v2;
}

struct memory_stat
{
    using timestamp_t = std::chrono::system_clock::time_point;

    /**
     * operations - The number of operations performed
     * bytes      - The number of bytes read or written
     * time       - The number of ns required for the operations
     * errors     - The number of errors during reading or writing
     */

    uint64_t operations = 0;
    uint64_t operations_target = 0;
    uint64_t bytes = 0;
    uint64_t bytes_target = 0;
    std::chrono::nanoseconds time = 0ns;
    std::optional<std::chrono::nanoseconds> latency_min;
    std::optional<std::chrono::nanoseconds> latency_max;
    timestamp_t timestamp = std::chrono::system_clock::now();
    uint64_t errors = 0;

    memory_stat& operator+=(const memory_stat& st)
    {
        bytes += st.bytes;
        bytes_target += st.bytes_target;
        errors += st.errors;
        operations += st.operations;
        operations_target += st.operations_target;
        time += st.time;
        timestamp = std::max(timestamp, st.timestamp);

        latency_min = bind_binary<std::chrono::nanoseconds>(
            [] (auto a, auto b) { return std::min(a, b); },
                latency_min, st.latency_min);

        latency_max = bind_binary<std::chrono::nanoseconds>(
            [] (auto a, auto b) { return std::max(a, b); },
                latency_max, st.latency_max);

        return *this;
    }

    memory_stat operator+(const memory_stat& st)
    {
        memory_stat stat(*this);
        return stat += st;
    }
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_STAT_
