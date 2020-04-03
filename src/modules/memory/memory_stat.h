#ifndef _OP_MEMORY_STAT_
#define _OP_MEMORY_STAT_

#include <cinttypes>
#include <limits>

namespace openperf::memory::internal {

struct memory_stat
{
    uint64_t operations = 0; //< The number of operations performed
    uint64_t operations_target = 0;
    uint64_t bytes = 0; //< The number of bytes read or written
    uint64_t bytes_target = 0;
    uint64_t errors = 0;  //< The number of errors during reading or writing
    uint64_t time_ns = 0; //< The number of ns required for the operations
    uint64_t latency_max = 0;
    uint64_t latency_min = std::numeric_limits<uint64_t>::max();
    uint64_t timestamp = 0;

    memory_stat& operator+=(const memory_stat& st)
    {
        bytes += st.bytes;
        bytes_target += st.bytes_target;
        errors += st.errors;
        operations += st.operations;
        operations_target += st.operations_target;
        time_ns += st.time_ns;
        latency_min = std::min(latency_min, st.latency_min);
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
