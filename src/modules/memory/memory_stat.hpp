#ifndef _OP_MEMORY_STAT_
#define _OP_MEMORY_STAT_

#include <cinttypes>
#include <limits>
#include <chrono>

namespace openperf::memory::internal {

using namespace std::chrono_literals;

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
    std::chrono::nanoseconds latency_min = std::chrono::nanoseconds::max();
    std::chrono::nanoseconds latency_max = std::chrono::nanoseconds::min();
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
        latency_min =
            std::min((latency_min != 0ns) ? latency_min : st.latency_min,
                     (st.latency_min != 0ns) ? st.latency_min : latency_min);
        latency_max = std::max(latency_max, st.latency_max);
        timestamp = std::max(timestamp, st.timestamp);

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
