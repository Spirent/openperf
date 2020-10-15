#ifndef _OP_CPU_TASK_CPU_SYSTEM_HPP_
#define _OP_CPU_TASK_CPU_SYSTEM_HPP_

#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <vector>

#include "framework/generator/pid_control.hpp"
#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"

#include "common.hpp"
#include "cpu.hpp"
#include "target.hpp"
#include "cpu_stat.hpp"
#include "target_scalar.hpp"

namespace openperf::cpu::internal {

using task_cpu_stat_ptr = std::unique_ptr<task_cpu_stat>;

class task_cpu_system
    : public openperf::framework::generator::task<task_cpu_stat_ptr>
{
    using target_ptr = std::unique_ptr<target>;
    using chronometer = ::openperf::timesync::chrono::monotime;
    using pid_control = ::openperf::framework::generator::pid_control;

private:
    task_cpu_config m_config;
    target_scalar<uint32_t> m_target;

    std::chrono::nanoseconds m_time;
    timesync::chrono::monotime::time_point m_last_run;
    utilization_time m_util_time;

    double m_utilization;
    uint16_t m_core_count;

    pid_control m_pid;
    std::chrono::nanoseconds m_available;
    utilization_time m_start;

public:
    task_cpu_system() = delete;
    task_cpu_system(task_cpu_system&&) noexcept;
    task_cpu_system(const task_cpu_system&) = delete;
    explicit task_cpu_system(uint16_t core,
                             uint16_t core_count,
                             double utilization);
    ~task_cpu_system() override = default;

    void reset() override;
    task_cpu_stat_ptr spin() override;

    task_cpu_config config() const { return m_config; }

private:
    template <typename Function> std::chrono::nanoseconds run_time(Function&&);
};

//
// Implementation
//

// Methods : private
template <typename Function>
std::chrono::nanoseconds task_cpu_system::run_time(Function&& function)
{
    auto start_time = cpu_thread_time();
    auto t1 = chronometer::now();
    function();
    auto thread_time = (cpu_thread_time() - start_time).utilization;
    auto time = chronometer::now() - t1;

    // We prefer processor thread time, but sometimes it
    // can returns zero, than we use software time.
    return (thread_time > 0ns) ? thread_time : time;
}

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TASK_CPU_SYSTEM_HPP_
