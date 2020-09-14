#ifndef _OP_CPU_STATS_HPP_
#define _OP_CPU_STATS_HPP_

#include <chrono>
#include <cinttypes>
#include <string>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

struct utilization_time
{
    std::chrono::nanoseconds user = 0ns;
    std::chrono::nanoseconds system = 0ns;
    std::chrono::nanoseconds steal = 0ns;
    std::chrono::nanoseconds utilization = 0ns;

    utilization_time operator-(const utilization_time& other) const
    {
        return {
            .user = user - other.user,
            .system = system - other.system,
            .steal = steal - other.steal,
            .utilization = utilization - other.utilization,
        };
    }

    utilization_time& operator-=(const utilization_time& other)
    {
        user -= other.user;
        system -= other.system;
        steal -= other.steal;
        utilization -= other.utilization;
        return *this;
    }

    utilization_time operator+(const utilization_time& other) const
    {
        return {
            .user = user + other.user,
            .system = system + other.system,
            .steal = steal + other.steal,
            .utilization = utilization + other.utilization,
        };
    }

    utilization_time& operator+=(const utilization_time& other)
    {
        user += other.user;
        system += other.system;
        steal += other.steal;
        utilization += other.utilization;
        return *this;
    }
};

utilization_time cpu_thread_time();
std::chrono::nanoseconds cpu_steal_time();

uint16_t cpu_cache_line_size();
std::string cpu_architecture();

} // namespace openperf::cpu::internal

#endif // _OP_CPU_STATS_HPP_
