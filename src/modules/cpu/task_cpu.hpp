#ifndef _OP_CPU_TASK_CPU_HPP_
#define _OP_CPU_TASK_CPU_HPP_

#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <vector>

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"

#include "common.hpp"
#include "cpu.hpp"
#include "target.hpp"
#include "cpu_stat.hpp"

namespace openperf::cpu::internal {

using task_cpu_stat_ptr = std::unique_ptr<task_cpu_stat>;

class task_cpu : public openperf::framework::generator::task<task_cpu_stat_ptr>
{
    using target_ptr = std::unique_ptr<target>;
    using chronometer = openperf::timesync::chrono::monotime;

    struct target_meta
    {
        uint64_t weight;
        std::chrono::nanoseconds runtime;
        target_ptr target;
    };

private:
    task_cpu_config m_config;

    uint64_t m_weights;
    uint64_t m_weight_min;
    std::chrono::nanoseconds m_time;
    std::chrono::nanoseconds m_error;
    timesync::chrono::monotime::time_point m_last_run;
    utilization_time m_util_time;
    std::vector<target_meta> m_targets;
    double m_utilization;

public:
    task_cpu() = delete;
    task_cpu(task_cpu&&) noexcept;
    task_cpu(const task_cpu&) = delete;
    explicit task_cpu(const task_cpu_config&);
    ~task_cpu() override = default;

    void reset() override;
    task_cpu_stat_ptr spin() override;

    task_cpu_config config() const { return m_config; }

private:
    void config(const task_cpu_config&);
    target_ptr make_target(cpu::instruction_set::type, cpu::data_type);
    template <typename Function> std::chrono::nanoseconds run_time(Function&&);
};

//
// Implementation
//

// Methods : private
template <typename Function>
std::chrono::nanoseconds task_cpu::run_time(Function&& function)
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

#endif // _OP_CPU_TASK_CPU_HPP_
