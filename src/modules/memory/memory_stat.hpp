#ifndef _OP_MEMORY_STAT_
#define _OP_MEMORY_STAT_

#include <cinttypes>
#include <chrono>
#include <optional>

#include "timesync/chrono.hpp"

namespace openperf::memory::internal {

using namespace std::chrono_literals;

struct task_memory_stat
{
    using clock = openperf::timesync::chrono::realtime;
    using timestamp_t = clock::time_point;
    using optional_time_t = std::optional<std::chrono::nanoseconds>;

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
    std::chrono::nanoseconds run_time = 0ns;
    std::chrono::nanoseconds sleep_time = 0ns;
    optional_time_t latency_min;
    optional_time_t latency_max;
    timestamp_t timestamp = clock::now();
    uint64_t errors = 0;

    task_memory_stat& operator+=(const task_memory_stat& st);
    task_memory_stat operator+(const task_memory_stat& st);
};

struct memory_stat
{
    bool active;
    task_memory_stat read;
    task_memory_stat write;

    task_memory_stat::timestamp_t timestamp() const;
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_STAT_
