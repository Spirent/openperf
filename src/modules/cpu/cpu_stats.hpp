#ifndef _OP_CPU_STATS_HPP_
#define _OP_CPU_STATS_HPP_

#include <chrono>

namespace openperf::cpu::internal {

struct utilization_time {
    std::chrono::nanoseconds user;
    std::chrono::nanoseconds system;
    std::chrono::nanoseconds steal;
};

utilization_time get_thread_utilization_time();
std::chrono::nanoseconds cpu_stats_get_steal_time();

} // namespace openperf::cpu::internal

#endif // _OP_CPU_STATS_HPP_
