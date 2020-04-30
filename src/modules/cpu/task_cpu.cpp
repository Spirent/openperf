#include "cpu/task_cpu.hpp"
#include "core/op_log.h"
#include "cpu/target_scalar.hpp"

#include <ctime>
#include <iostream>

namespace openperf::cpu::internal {

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
    for (auto& target : m_targets) {
        uint64_t calls = CLOCKS_PER_SEC * m_utilization
            / target.runtime * target.weight;

        std::cout << "Calls: " << calls << std::endl;
        uint64_t cycles = 0;
        for (uint64_t i = 0; i < calls; ++i)
            cycles += target.target->operation();

        m_stat_active.targets[counter++].cycles += cycles;
    }

    m_stat.store(&m_stat_active);
    m_stat_shared = m_stat_active;
    m_stat.store(&m_stat_shared);
}

void task_cpu::config(const task_cpu::config_t& conf)
{
    OP_LOG(OP_LOG_INFO, "Set config\n");
    std::cout << "Utilization: " << conf.utilization << std::endl;

    m_weights = 0;
    m_stat_clear = true;
    m_targets_count = 0;
    m_utilization = conf.utilization / 100;

    std::cout << "Config size: " << conf.targets.size() << std::endl;
    m_targets.clear();
    for (auto& t_conf : conf.targets) {
        ++m_targets_count;
        m_weights += t_conf.weight;

        std::cout << "Config: { weight: "
            << t_conf.weight << ", operation: "
            << (int)t_conf.operation << ", data_size: "
            << (int)t_conf.data_size << ", set: "
            << (int)t_conf.set
            << " }" << std::endl;

        auto meta = target_meta{};
        switch (t_conf.set)
        {
        case cpu::instruction_set::SCALAR:
            meta.target = std::make_unique<target_scalar>(
                t_conf.data_size, t_conf.operation);
            break;
        default:
            throw std::runtime_error("Unknown instruction set "
                + std::to_string((int)t_conf.set));
        }

        // Measure the target time, needed for planning
        auto start_t = std::clock();
        meta.target.get()->operation();
        meta.runtime = std::clock() - start_t;
        meta.weight = t_conf.weight;

        std::cout << "Task time: " << meta.runtime << std::endl;
        std::cout << "Task rate: " << meta.weight/meta.runtime << std::endl;

        m_targets.emplace_front(std::move(meta));
    }
}

task_cpu::stat_t task_cpu::stat() const
{
    return (m_stat_clear)
        ? stat_t{} : *m_stat;
}

} // namespace openperf::cpu::internal
