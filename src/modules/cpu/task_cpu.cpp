#include "cpu/task_cpu.hpp"
#include "core/op_log.h"
#include "cpu/target_scalar.hpp"


namespace openperf::cpu::internal
{

task_cpu::task_cpu()
    : m_stat(&m_stat_shared)
    , m_stat_clear(false)
{}

task_cpu::task_cpu(const task_cpu::config_t& conf)
    : task_cpu()
{
    config(conf);
}

void task_cpu::update_shared_stat()
{
    m_stat.store(&m_stat_active);
    m_stat_shared = m_stat_active;
    m_stat.store(&m_stat_shared);
}

void task_cpu::spin()
{
    if (m_stat_clear) {
        m_stat_active = {};
        update_shared_stat();
        m_stat_clear = false;
    }

    size_t counter = 0;
    for (auto& target : m_targets) {
        m_stat_active.targets[counter++].cycles += target->operation();
    }

    update_shared_stat();
}

void task_cpu::config(const task_cpu::config_t& conf)
{
    OP_LOG(OP_LOG_INFO, "Set config\n");

    m_targets.clear();
    m_stat_active.targets.clear();
    for (auto& t_conf : conf.targets) {
        m_stat_active.targets.push_back({});

        switch (t_conf.set)
        {
        case instruction_set::SCALAR:
            m_targets.emplace_front(
                std::make_unique<target_scalar>(
                    target_scalar{
                        t_conf.data_size,
                        t_conf.operation,
                        t_conf.weight
                    }
            ));
            break;
        }
    }
    update_shared_stat();
}

task_cpu::stat_t task_cpu::stat() const
{
    return (m_stat_clear)
        ? stat_t{} : *m_stat;
}

} // namespace openperf::cpu::internal