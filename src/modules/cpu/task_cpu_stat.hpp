#ifndef _OP_CPU_TASK_CPU_STAT_HPP_
#define _OP_CPU_TASK_CPU_STAT_HPP_

#include <cinttypes>
#include <vector>

namespace openperf::cpu::internal {

struct target_cpu_stat {
    uint64_t cycles;
    uint64_t ticks;
    std::chrono::nanoseconds runtime;
};

struct task_cpu_stat {
    uint64_t available = 0;
    uint64_t utilization = 0;
    uint64_t system = 0;
    uint64_t user = 0;
    uint64_t steal = 0;
    uint64_t error = 0;
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
