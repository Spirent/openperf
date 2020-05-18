#include "task_cpu.hpp"

#include "core/op_log.h"
#include "cpu/target_scalar.hpp"

#include <thread>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

auto now = openperf::timesync::chrono::monotime::now;

constexpr auto QUANTA = 100ms;

task_cpu::task_cpu()
    : m_stat(&m_stat_shared)
    , m_stat_clear(true)
    , m_utilization(0.0)
{}

task_cpu::task_cpu(const task_cpu::config_t& conf)
    : task_cpu()
{
    config(conf);
}

[[clang::optnone]]
void task_cpu::spin()
{
    if (m_stat_clear) {
        m_stat_active = {};
        m_stat_shared = {};
        m_stat_active.targets.resize(m_targets.size());
        m_stat_shared.targets.resize(m_targets.size());
        m_stat_clear = false;

        m_error = 0ns;
        m_last_run = now();
        m_util_time = cpu_thread_time();
    }

    auto time_frame = std::max(
        m_time.count() * (double)m_weights / m_weight_min,
        std::chrono::duration_cast<std::chrono::nanoseconds>(QUANTA).count()
            * m_utilization);

    m_time = 0ns;
    for (size_t i = 0; i < m_targets.size(); ++i) {
        auto& target = m_targets.at(i);

        // Calculate number of calls for available time_frame
        // and weight of target
        uint64_t calls = time_frame / m_weights
            * target.weight / target.runtime.count();

        uint64_t cycles = 0;
        auto runtime = run_time([&](){
            for (uint64_t c = 0; c < calls; ++c)
                cycles += target.target->operation();
        });

        target.runtime = (target.runtime + runtime / calls) / 2;
        m_time += target.runtime;

        auto& stat = m_stat_active.targets[i];
        stat.cycles += cycles;
        stat.runtime += runtime;
    }

    auto cpu_util = cpu_thread_time();
    auto time_diff = cpu_util - m_util_time;

    std::this_thread::sleep_for(
        (time_diff.utilization - m_error)
        * (1.0 - m_utilization) / m_utilization);

    auto time_of_run = now();
    auto available = time_of_run - m_last_run;

    m_error = std::chrono::nanoseconds(
        (uint64_t)(available.count() * m_utilization))
            - time_diff.utilization + m_error;

    m_stat_active.system += time_diff.system;
    m_stat_active.user += time_diff.user;
    m_stat_active.steal += time_diff.steal;
    m_stat_active.utilization += time_diff.utilization;
    m_stat_active.available += available;

    m_stat_active.error = std::chrono::nanoseconds(
        (uint64_t)(m_stat_active.available.count() * m_utilization))
            - m_stat_active.utilization;
    m_stat_active.load = m_stat_active.utilization * 1.0
        / m_stat_active.available;

    m_last_run = time_of_run;
    m_util_time = cpu_util;
    m_stat.store(&m_stat_active);
    m_stat_shared = m_stat_active;
    m_stat.store(&m_stat_shared);
}

[[clang::optnone]]
void task_cpu::config(const task_cpu::config_t& conf)
{
    assert(0.0 < conf.utilization && conf.utilization <= 100.0);

    m_stat_clear = true;
    m_utilization = conf.utilization / 100.0;

    m_time = 0ns;
    m_weights = 0;
    m_weight_min = std::numeric_limits<uint64_t>::max();
    m_targets.clear();
    for (auto& t_conf : conf.targets) {
        auto meta = target_meta{};
        meta.weight = t_conf.weight;
        meta.target = make_target(t_conf.set, t_conf.data_type);

        // Measure the target run time, needed for planning
        auto runs_for_measure = 5;
        meta.runtime = run_time([&](){
            for (auto i = 0; i < runs_for_measure; ++i)
                meta.target.get()->operation();
        }) / runs_for_measure;

        m_time += meta.runtime;
        m_weights += t_conf.weight;
        m_weight_min = std::min(m_weight_min, meta.weight);
        m_targets.emplace_back(std::move(meta));
    }
}

task_cpu::stat_t task_cpu::stat() const
{
    return (m_stat_clear)
        ? stat_t{} : *m_stat;
}

task_cpu::target_ptr task_cpu::make_target(cpu::instruction_set iset, cpu::data_type dtype)
{
    switch (iset) {
    case cpu::instruction_set::SCALAR:
        switch (dtype) {
        case cpu::data_type::INT32:
            return std::make_unique<target_scalar<uint32_t>>(dtype);
        case cpu::data_type::INT64:
            return std::make_unique<target_scalar<uint64_t>>(dtype);
        case cpu::data_type::FLOAT32:
            return std::make_unique<target_scalar<float>>(dtype);
        case cpu::data_type::FLOAT64:
            return std::make_unique<target_scalar<double>>(dtype);
        }
    default:
        throw std::runtime_error("Unknown instruction set "
            + std::to_string((int)iset));
    }
}

[[clang::optnone]]
std::chrono::nanoseconds task_cpu::run_time(std::function<void ()> function)
{
    auto start_time = cpu_thread_time();
    auto t1 = now();
    function();
    auto thread_time = (cpu_thread_time() - start_time).utilization;
    auto time = now() - t1;

    // We prefer processor thread time, but sometimes it
    // can returns zero, than we use software time.
    return (thread_time > 0ns) ? thread_time : time;
}

} // namespace openperf::cpu::internal
