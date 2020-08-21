#include "cpu_stat.hpp"

#include <cassert>

namespace openperf::cpu {

task_cpu_stat::task_cpu_stat(size_t targets_number)
    : targets(targets_number)
{}

task_cpu_stat& task_cpu_stat::operator+=(const task_cpu_stat& other)
{
    assert(targets.size() == other.targets.size());

    available += other.available;
    utilization += other.utilization;
    system += other.system;
    user += other.user;
    steal += other.steal;
    error += other.error;

    for (size_t i = 0; i < targets.size(); i++) {
        targets[i].operations += other.targets[i].operations;
        targets[i].runtime += other.targets[i].runtime;
    }

    return *this;
}

task_cpu_stat task_cpu_stat::operator+(const task_cpu_stat& other) const
{
    auto copy = *this;
    return copy += other;
}

void task_cpu_stat::clear()
{
    available = 0ns;
    utilization = 0ns;
    system = 0ns;
    user = 0ns;
    steal = 0ns;
    error = 0ns;

    for (auto&& target : targets) target = {};
}

cpu_stat::cpu_stat(size_t cores_number)
    : cores(cores_number)
{}

cpu_stat& cpu_stat::operator+=(const task_cpu_stat& task)
{
    assert(task.core < cores.size());

    available += task.available;
    utilization += task.utilization;
    system += task.system;
    user += task.user;
    steal += task.steal;
    error += task.error;

    cores[task.core] += task;

    return *this;
}

cpu_stat cpu_stat::operator+(const task_cpu_stat& task) const
{
    auto stat = *this;
    return stat += task;
}

void cpu_stat::clear()
{
    available = 0ns;
    utilization = 0ns;
    system = 0ns;
    user = 0ns;
    steal = 0ns;
    error = 0ns;

    for (auto&& core : cores) core.clear();
}

} // namespace openperf::cpu
