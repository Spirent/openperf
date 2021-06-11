#include "task_cpu.hpp"
#include "target_scalar.hpp"
#include "target_ispc.hpp"

#include "framework/core/op_log.h"

#include <thread>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

constexpr auto QUANTA = 100ms;

// Constructors & Destructor
task_cpu::task_cpu(const task_cpu_config& conf) { config(conf); }

task_cpu::task_cpu(task_cpu&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_weights(other.m_weights)
    , m_weight_min(other.m_weight_min)
    , m_time(other.m_time)
    , m_error(other.m_error)
    , m_last_run(other.m_last_run)
    , m_util_time(other.m_util_time)
    , m_targets(std::move(other.m_targets))
    , m_utilization(other.m_utilization)
{}

void task_cpu::reset()
{
    m_error = 0ns;
    m_last_run = chronometer::time_point();
}

// Methods : public
task_cpu_stat_ptr task_cpu::spin()
{
    if (m_last_run.time_since_epoch() == 0ns) {
        m_last_run = chronometer::now();
        m_util_time = cpu_thread_time();
    }

    auto time_frame = static_cast<uint64_t>(std::max(
        m_time.count() * static_cast<double>(m_weights) / m_weight_min,
        std::chrono::duration_cast<std::chrono::nanoseconds>(QUANTA).count()
            * m_utilization));

    auto stat = task_cpu_stat{m_targets.size()};

    m_time = 0ns;
    for (size_t i = 0; i < m_targets.size(); ++i) {
        auto& target = m_targets.at(i);

        // Calculate number of calls for available time_frame
        // and weight of target
        uint64_t calls =
            time_frame / m_weights * target.weight / target.runtime.count();

        uint64_t operations = 0;
        auto runtime = run_time([&]() {
            for (uint64_t c = 0; c < calls; ++c)
                operations += target.target->operation();
        });

        target.runtime = (target.runtime + runtime / calls) / 2;
        m_time += target.runtime;

        auto& tgt_stat = stat.targets[i];
        tgt_stat.operations += operations;
        tgt_stat.runtime += runtime;
    }

    auto cpu_util = cpu_thread_time();
    auto time_diff = cpu_util - m_util_time;

    std::this_thread::sleep_for((time_diff.utilization - m_error)
                                * (1.0 - m_utilization) / m_utilization);

    auto time_of_run = chronometer::now();
    auto available = time_of_run - m_last_run;

    m_error =
        std::chrono::nanoseconds((uint64_t)(available.count() * m_utilization))
        - time_diff.utilization + m_error;

    stat.available = available;
    stat.utilization = time_diff.utilization;
    stat.system = time_diff.system;
    stat.user = time_diff.user;
    stat.steal = time_diff.steal;
    stat.core = m_config.core;

    stat.target = std::chrono::duration_cast<std::chrono::nanoseconds>(
        stat.available * m_utilization);
    stat.error = stat.target - stat.utilization;
    stat.load = stat.utilization * 1.0 / stat.available;

    m_last_run = time_of_run;
    m_util_time = cpu_util;

    // NOTE: clang-tidy static analysis cause an error when used simply:
    // return std::make_unique<task_cpu_stat>();
    //
    // Link: https://bugs.llvm.org/show_bug.cgi?id=38176
    // should be fixed in clang 8 or newer
    auto return_value = std::make_unique<task_cpu_stat>(stat);
    return return_value;
}

// Methods : private
void task_cpu::config(const task_cpu_config& conf)
{
    assert(0.0 < conf.utilization && conf.utilization <= 100.0);
    OP_LOG(OP_LOG_DEBUG, "CPU Task configuring");

    m_time = 0ns;
    m_error = 0ns;
    m_weights = 0;
    m_weight_min = std::numeric_limits<uint64_t>::max();
    m_utilization = conf.utilization / 100.0;
    m_config = conf;

    m_targets.clear();
    for (const auto& t_conf : conf.targets) {
        auto meta = target_meta{};
        meta.weight = t_conf.weight;
        meta.target = make_target(t_conf.set, t_conf.data_type);

        // Measure the target run time, needed for planning
        auto runs_for_measure = 5;
        meta.runtime = run_time([&]() {
                           for (auto i = 0; i < runs_for_measure; ++i)
                               meta.target.get()->operation();
                       })
                       / runs_for_measure;

        m_time += meta.runtime;
        m_weights += t_conf.weight;
        m_weight_min = std::min(m_weight_min, meta.weight);
        m_targets.emplace_back(std::move(meta));
    }
}

} // namespace openperf::cpu::internal
