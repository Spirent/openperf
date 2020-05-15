#include "cpu/task_cpu.hpp"
#include "core/op_log.h"
#include "cpu/target_scalar.hpp"
#include "cpu/cpu_stats.hpp"

#include <ctime>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "timesync/chrono.hpp"

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

auto now = openperf::timesync::chrono::monotime::now;

constexpr auto QUANTA = 100ms;

task_cpu::task_cpu()
    : m_stat(&m_stat_shared)
    , m_stat_clear(false)
    , m_targets_count(0)
    , m_utilization(0.0)
{}

task_cpu::task_cpu(const task_cpu::config_t& conf)
    : task_cpu()
{
    config(conf);
}

void task_cpu::spin()
{
    if (m_stat_clear) {
        m_stat_active = {};
        m_stat_shared = {};
        m_stat_active.targets.resize(m_targets_count);
        m_stat_shared.targets.resize(m_targets_count);
        m_stat_clear = false;
    }

    size_t counter = 0;
    auto overall_time = 0ns;
    auto overall_ticks = 0;
    auto t1 = now();
    auto time_frame = std::max(
        m_time.count() * (double)m_weights / m_weight_min,
        std::chrono::duration_cast<std::chrono::nanoseconds>(QUANTA).count()
            * m_utilization);

    std::cout << "All Time: " << m_time.count() << "ns"
        << ", quanta: " << time_frame << "ns" << std::endl;

    m_time = 0ns;
    m_ticks = 0;
    for (auto& target : m_targets) {
        uint64_t calls = time_frame / m_weights
            * target.weight / target.runtime.count();

        uint64_t cycles = 0;
        auto start_ts = now();
        auto start_t = std::clock();
        for (uint64_t i = 0; i < calls; ++i) {
            cycles += target.target->operation();
        }
        auto ticks = std::clock() - start_t;
        auto time = now() - start_ts;

        overall_ticks += ticks;
        overall_time += time;

        std::cout << "[" << counter << ":" << target.weight
            << "] tk: " << std::setw(4) << target.ticks
            << ", rt: " << std::setw(6) << target.runtime.count()
            << ", call " << std::setw(5) << calls
            << " times, time: " << std::setw(6) << ticks
            << " ticks (" << std::setw(4) << ticks * 1000.0 / CLOCKS_PER_SEC << "ms) "
            << std::setw(4) << std::chrono::duration_cast<std::chrono::microseconds>(time).count() << "mcs"
            << std::endl;

        target.ticks = (target.ticks + ticks / calls) / 2;
        target.runtime = (target.runtime + time / calls) / 2;

        m_ticks += target.ticks;
        m_time += target.runtime;

        auto& stat = m_stat_active.targets[counter++];
        stat.cycles += cycles;
        stat.ticks += ticks;
        stat.runtime += time;
    }

    auto cpu_util = get_thread_utilization_time();
    m_stat_active.system = cpu_util.system;
    m_stat_active.user = cpu_util.user;
    m_stat_active.steal = cpu_util.steal;
    //m_stat_active.steal += cpu_util.steal;
    //m_stat_active.available = cpu_stat.
    //m_stat_active.utilization =
    //m_stat_active.error = cpu_stat.


    //std::cout << "Utilization time {sys, usr}: { "
    //    << utime.system.count()
    //    << ", " << utime.user.count() << " }"
    //    << std::endl;
    std::cout << "Utilization time {sys, usr, steal}: { "
        << cpu_util.system.count() << " "
        << cpu_util.user.count() << " "
        << cpu_util.steal.count() << std::endl;

    auto sleep_time = (now() - t1) * (1.0 - m_utilization) / m_utilization;

    std::cout << "Ticks: " << std::setw(9) << overall_ticks
        << "/" << CLOCKS_PER_SEC << " ("
        << std::setw(4) << overall_ticks * 1000.0 / CLOCKS_PER_SEC << "ms) "
        << std::setw(4) <<
            std::chrono::duration_cast<std::chrono::microseconds>(overall_time).count() << "mcs"
        << std::endl;

    std::cout << "Sleep time: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time).count()
        << "ms" << std::endl;

    std::this_thread::sleep_for(sleep_time);

    m_stat.store(&m_stat_active);
    m_stat_shared = m_stat_active;
    m_stat.store(&m_stat_shared);
}

void task_cpu::config(const task_cpu::config_t& conf)
{
    OP_LOG(OP_LOG_INFO, "CPU Task configuring\n");

    m_stat_clear = true;
    m_weights = 0;
    m_weight_min = std::numeric_limits<uint64_t>::max();
    m_ticks = 0;
    m_time = 0ns;
    m_targets_count = 0;
    m_utilization = conf.utilization / 100;

    std::cout << "Config size: " << conf.targets.size() << std::endl;
    m_targets.clear();
    for (auto& t_conf : conf.targets) {
        ++m_targets_count;

        std::cout << "Config: { weight: "
            << t_conf.weight << ", operation: "
            << (int)t_conf.data_type << ", set: "
            << (int)t_conf.set
            << " }" << std::endl;

        auto meta = target_meta{};
        switch (t_conf.set)
        {
        case cpu::instruction_set::SCALAR:
            switch (t_conf.data_type)
            {
            case cpu::data_type::INT32:
                meta.target =
                    std::make_unique<target_scalar<uint32_t>>(
                        t_conf.data_type);
                break;
            case cpu::data_type::INT64:
                meta.target =
                    std::make_unique<target_scalar<uint64_t>>(
                        t_conf.data_type);
                break;
            case cpu::data_type::FLOAT32:
                meta.target =
                    std::make_unique<target_scalar<float>>(
                        t_conf.data_type);
                break;
            case cpu::data_type::FLOAT64:
                meta.target =
                    std::make_unique<target_scalar<double>>(
                        t_conf.data_type);
                break;
            }
            break;
        default:
            throw std::runtime_error("Unknown instruction set "
                + std::to_string((int)t_conf.set));
        }

        // Measure the target time, needed for planning
        auto start_ts = now();
        auto start_t = std::clock();
        meta.target.get()->operation();
        meta.ticks = std::clock() - start_t;
        meta.runtime = now() - start_ts;
        meta.weight = t_conf.weight;

        std::cout << "Task time: " <<
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                meta.runtime).count() << std::endl
            << "Task time: " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(
                meta.runtime).count() << std::endl
            << "Task ticks: " << meta.ticks << std::endl
            << "Time ticks: " << meta.ticks * 1000.0 / CLOCKS_PER_SEC << std::endl
            << "Task rate: " << meta.weight/(double)meta.ticks << std::endl;

        m_ticks += meta.ticks;
        m_time += meta.runtime;
        m_weights += t_conf.weight;
        m_weight_min = std::min(m_weight_min, meta.weight);
        m_targets.emplace_front(std::move(meta));
    }

    std::cout << "Ticks: " << m_ticks << ", Time: "
        << m_time.count() << "ns" << std::endl;
}

task_cpu::stat_t task_cpu::stat() const
{
    return (m_stat_clear)
        ? stat_t{} : *m_stat;
}

} // namespace openperf::cpu::internal
