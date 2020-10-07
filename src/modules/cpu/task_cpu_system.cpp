#include "task_cpu_system.hpp"

#include "framework/core/op_log.h"

#include <thread>
#include <iomanip>

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

constexpr auto QUANTA = 100ms;

// Constructors & Destructor
task_cpu_system::task_cpu_system(uint16_t core,
                                 uint16_t core_count,
                                 double utilization)
    : m_utilization(utilization / 100.0)
    , m_core_count(core_count)
    , m_pid(0.9, 0.0005, 0.000)
{
    assert(0.0 < utilization && utilization <= 100.0);
    OP_LOG(OP_LOG_DEBUG, "CPU Task configuring");

    m_config = {
        .utilization = utilization,
        .core = core,
        .targets =
            {
                1,
                target_config{
                    .data_type = cpu::data_type::INT32,
                    .set = cpu::instruction_set::SCALAR,
                    .weight = 1,
                },
            },
    };

    // System Load
    m_pid.reset(m_utilization);
    m_pid.max(1.0);
    m_pid.min(0.0);
    m_available = 0ns;

    // Measure the target run time, needed for planning
    auto runs_for_measure = 5;
    m_time = run_time([&]() {
                 for (auto i = 0; i < runs_for_measure; ++i)
                     m_target.operation();
             })
             / runs_for_measure;
}

task_cpu_system::task_cpu_system(task_cpu_system&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_time(other.m_time)
    //, m_error(other.m_error)
    , m_last_run(other.m_last_run)
    , m_util_time(other.m_util_time)
    , m_utilization(other.m_utilization)
    , m_core_count(other.m_core_count)
    , m_pid(other.m_pid)
{}

void task_cpu_system::reset()
{
    // m_error = 0ns;
    m_last_run = chronometer::time_point();
}

// Methods : public
task_cpu_stat_ptr task_cpu_system::spin()
{
    if (m_last_run.time_since_epoch() == 0ns) {
        m_last_run = chronometer::now();
        m_util_time = cpu_thread_time();
        m_start = cpu_process_time();
        m_pid.start();
    }

    auto time_frame = static_cast<uint64_t>(std::max(
        static_cast<double>(m_time.count()),
        std::chrono::duration_cast<std::chrono::nanoseconds>(QUANTA).count()
            * m_utilization));

    // Calculate number of calls for available time_frame
    uint64_t calls = time_frame / m_time.count();

    uint64_t operations = 0;
    auto runtime = run_time([&]() {
        for (uint64_t call = 0; call < calls; ++call)
            operations += m_target.operation();
    });

    m_time = (m_time + runtime / calls) / 2;

    auto stat = task_cpu_stat{1};
    auto& tgt_stat = stat.targets[0];
    tgt_stat.operations += operations;
    tgt_stat.runtime += runtime;

    auto cpu_util = cpu_thread_time();
    auto time_diff = cpu_util - m_util_time;

    std::this_thread::sleep_for(time_diff.utilization * (1.0 - m_utilization)
                                / m_utilization);
    // std::this_thread::sleep_for((time_diff.utilization - m_error)
    //                            * (1.0 - m_utilization) / m_utilization);

    auto time_of_run = chronometer::now();
    auto available = time_of_run - m_last_run;

    // m_error =
    //    std::chrono::nanoseconds((uint64_t)(available.count() *
    //    m_utilization))
    //    - time_diff.utilization + m_error;

    stat.available = available;
    stat.utilization = time_diff.utilization;
    stat.system = time_diff.system;
    stat.user = time_diff.user;
    stat.steal = time_diff.steal;
    stat.core = m_config.core;

    stat.error = std::chrono::nanoseconds(
                     (uint64_t)(stat.available.count() * m_utilization))
                 - stat.utilization;
    stat.load = stat.utilization * 1.0 / stat.available;

    m_last_run = time_of_run;
    m_util_time = cpu_util;

    // m_error = 0ns;
    m_available += stat.available;

    auto cpu_time = internal::cpu_process_time() - m_start;
    auto cpu_actual = cpu_time.utilization - cpu_time.steal;
    if (cpu_actual < 0ns) cpu_actual = 0ns;

    auto util = static_cast<double>(cpu_actual.count())
                / (m_core_count * m_available.count());
    auto adjust = m_pid.stop(util);

    // if (m_config.core == 1) {
    //    std::cout << "CPU_ACT: " << cpu_actual.count()
    //              << ", M_AVAIL: " << m_available.count()
    //              << ", CORES: " << m_core_count << std::endl;
    //    std::cout << std::setw(2) << m_config.core
    //              << " > REAL UTILIZATION: " << util * 100.0
    //              << ", LOAD: " << stat.load * 100.0
    //              << ", UTIL: " << m_utilization * 100.0 << " -> "
    //              << m_config.utilization << " + " << adjust * 100.0 << " -> "
    //              << m_config.utilization + adjust * 100.0
    //              << ", ADJ: " << adjust << std::endl;
    //}
    m_utilization = m_config.utilization / 100.0 + adjust;
    m_pid.start();

    // NOTE: clang-tidy static analysis cause an error when used simply:
    // return std::make_unique<task_cpu_stat>();
    //
    // Link: https://bugs.llvm.org/show_bug.cgi?id=38176
    // should be fixed in clang 8 or newer
    auto return_value = std::make_unique<task_cpu_stat>(stat);
    return return_value;
}

} // namespace openperf::cpu::internal
