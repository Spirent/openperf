#include "cpu_stat.hpp"

#include <cassert>

namespace openperf::cpu {

task_cpu_stat::task_cpu_stat(size_t targets_number)
    : targets(targets_number)
{}

task_cpu_stat& task_cpu_stat::operator+=(const task_cpu_stat& other)
{
    available += other.available;
    utilization += other.utilization;
    system += other.system;
    user += other.user;
    steal += other.steal;
    error += other.error;

    targets.resize(std::max(targets.size(), other.targets.size()));
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

cpu_stat::cpu_stat(size_t cores_number)
    : cores(cores_number)
{}

cpu_stat& cpu_stat::operator+=(const task_cpu_stat& task)
{
    // if (task.core >= cores.size())
    //
    // std::cout << "WARNING: " << task.core << " " << cores.size() <<
    // std::endl;
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

} // namespace openperf::cpu
