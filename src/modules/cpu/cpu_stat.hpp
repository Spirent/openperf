#ifndef _OP_CPU_TASK_CPU_STAT_HPP_
#define _OP_CPU_TASK_CPU_STAT_HPP_

#include <chrono>
#include <cinttypes>
#include <vector>

namespace openperf::cpu {

using namespace std::chrono_literals;

struct common_stat
{
    std::chrono::nanoseconds available = 0ns;
    std::chrono::nanoseconds utilization = 0ns;
    std::chrono::nanoseconds system = 0ns;
    std::chrono::nanoseconds user = 0ns;
    std::chrono::nanoseconds steal = 0ns;
    std::chrono::nanoseconds error = 0ns;
};

struct task_cpu_stat : public common_stat
{
    struct target_cpu_stat
    {
        uint64_t operations = 0;
        std::chrono::nanoseconds runtime = 0ns;
    };

    double load = 0.0;
    uint16_t core = 0;
    std::vector<target_cpu_stat> targets;

    task_cpu_stat(size_t targets = 0);
    task_cpu_stat& operator+=(const task_cpu_stat&);
    task_cpu_stat operator+(const task_cpu_stat&) const;

    void clear();
};

struct cpu_stat : public common_stat
{
    std::vector<task_cpu_stat> cores;

    cpu_stat(size_t cores = 0);
    cpu_stat& operator+=(const task_cpu_stat&);
    cpu_stat operator+(const task_cpu_stat&) const;

    void clear();
};

} // namespace openperf::cpu

#endif // _OP_CPU_TASK_CPU_STAT_HPP_
