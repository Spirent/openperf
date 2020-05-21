#ifndef _OP_CPU_TASK_CPU_STAT_HPP_
#define _OP_CPU_TASK_CPU_STAT_HPP_

#include <chrono>
#include <cinttypes>
#include <vector>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

struct target_cpu_stat {
    uint64_t operations;
    uint64_t ticks;
    std::chrono::nanoseconds runtime;
};

struct task_cpu_stat {
    std::chrono::nanoseconds available = 0ns;
    std::chrono::nanoseconds utilization = 0ns;
    std::chrono::nanoseconds system = 0ns;
    std::chrono::nanoseconds user = 0ns;
    std::chrono::nanoseconds steal = 0ns;
    std::chrono::nanoseconds error = 0ns;
    double load = 0.0;

    std::vector<target_cpu_stat> targets;

    task_cpu_stat() = default;

    task_cpu_stat(int targets_number)
    {
        targets.resize(targets_number);
    }

    task_cpu_stat(const task_cpu_stat& other)
        : available(other.available)
        , utilization(other.utilization)
        , system(other.system)
        , user(other.user)
        , steal(other.steal)
        , error(other.error)
    {
        targets.resize(other.targets.size());
        for (size_t i = 0; i < other.targets.size(); ++i)
            targets[i] = other.targets[i];
    }

    task_cpu_stat& operator= (const task_cpu_stat& other) {
        available = other.available;
        utilization = other.utilization;
        system = other.system;
        user = other.user;
        steal = other.steal;
        error = other.error;

        targets.resize(other.targets.size());
        for (size_t i = 0; i < other.targets.size(); ++i)
            targets[i] = other.targets[i];

        return *this;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TASK_CPU_STAT_HPP_
