#ifndef _OP_CPU_GENERATOR_SYSTEM_STATS_HPP_
#define _OP_CPU_GENERATOR_SYSTEM_STATS_HPP_

#include <chrono>
#include <utility>

#include <sys/time.h>

namespace openperf::cpu::generator {

std::chrono::microseconds get_steal_time();
std::pair<timeval, timeval> get_thread_cpu_usage();

template <typename Clock> struct utilization_time
{
    typename Clock::time_point time_stamp;
    typename Clock::duration time_system;
    typename Clock::duration time_user;
};

template <typename Duration> Duration to_duration(const timeval& tv)
{
    return (std::chrono::seconds{tv.tv_sec}
            + std::chrono::microseconds{tv.tv_usec});
}

template <typename Clock> utilization_time<Clock> get_thread_utilization_time()
{
    auto [system, user] = get_thread_cpu_usage();
    return (utilization_time<Clock>{
        .time_stamp = Clock::now(),
        .time_system = to_duration<typename Clock::duration>(system),
        .time_user = to_duration<typename Clock::duration>(user)});
}

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_CPU_STEAL_HPP_ */
