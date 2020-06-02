#ifndef _OP_CPU_TASK_CPU_HPP_
#define _OP_CPU_TASK_CPU_HPP_

#include <atomic>
#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <vector>

#include "cpu/common.hpp"
#include "cpu/cpu.hpp"
#include "cpu/target.hpp"
#include "cpu/task_cpu_stat.hpp"
#include "timesync/chrono.hpp"
#include "utils/worker/task.hpp"

namespace openperf::cpu::internal {

struct target_config
{
    cpu::instruction_set set;
    cpu::data_type data_type;
    uint64_t weight;
};

struct task_cpu_config
{
    double utilization = 0.0;
    std::vector<target_config> targets;
    task_cpu_config() = default;
    task_cpu_config(const task_cpu_config& other)
        : utilization(other.utilization)
    {
        for (size_t i = 0; i < other.targets.size(); ++i)
            targets.push_back(other.targets[i]);
    }
};

class task_cpu : public utils::worker::task<task_cpu_config, task_cpu_stat>
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
    config_t m_config;
    stat_t m_stat_shared, m_stat_active;

    std::atomic<stat_t*> m_stat;
    std::atomic_bool m_stat_clear;

    uint64_t m_weights;
    uint64_t m_weight_min;
    std::chrono::nanoseconds m_time;
    std::chrono::nanoseconds m_error;
    timesync::chrono::monotime::time_point m_last_run;
    utilization_time m_util_time;
    std::vector<target_meta> m_targets;
    double m_utilization;

public:
    task_cpu();
    explicit task_cpu(const config_t&);
    ~task_cpu() override = default;

    void spin() override;

    void config(const config_t&) override;
    config_t config() const override { return m_config; }

    stat_t stat() const override;
    void clear_stat() override { m_stat_clear = true; }

private:
    target_ptr make_target(cpu::instruction_set, cpu::data_type);

    template <typename Function> std::chrono::nanoseconds run_time(Function&&);
};

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
