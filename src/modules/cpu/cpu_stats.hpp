#ifndef _OP_CPU_STATS_HPP_
#define _OP_CPU_STATS_HPP_

#include <chrono>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

struct utilization_time {
    std::chrono::nanoseconds user = 0ns;
    std::chrono::nanoseconds system = 0ns;
    std::chrono::nanoseconds steal = 0ns;
    std::chrono::nanoseconds utilization = 0ns;

    utilization_time operator-(const utilization_time& other) const {
        return {
            .user = user - other.user,
            .system = system - other.system,
            .steal = steal - other.steal,
            .utilization = utilization - other.utilization,
        };
    }

    utilization_time& operator-=(const utilization_time& other) {
        user -= other.user;
        system -= other.system;
        steal -=  other.steal;
        utilization -= other.utilization;
        return *this;
    }

    utilization_time operator+(const utilization_time& other) const {
        return {
            .user = user + other.user,
            .system = system + other.system,
            .steal = steal + other.steal,
            .utilization = utilization + other.utilization,
        };
    }

    utilization_time& operator+=(const utilization_time& other) {
        user += other.user;
        system += other.system;
        steal +=  other.steal;
        utilization += other.utilization;
        return *this;
    }
};

utilization_time get_thread_time();
std::chrono::nanoseconds get_steal_time();

} // namespace openperf::cpu::internal

#endif // _OP_CPU_STATS_HPP_
