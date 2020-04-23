#include "temp_task.hpp"
#include "core/op_log.h"

namespace openperf::cpu::worker
{

cpu_task::cpu_task()
    : m_actual_stat({})
    , m_shared_stat(m_actual_stat)
    , m_at_stat(&m_shared_stat)
    , m_reset_stat(false)
{}

cpu_task::~cpu_task() {}

void cpu_task::spin()
{
    if (m_reset_stat.load()) {
        m_reset_stat = false;
        m_actual_stat = *m_at_stat;
    }
    ++m_actual_stat.cycles;
    if (m_reset_stat.load()) {
        m_reset_stat = false;
        m_actual_stat = *m_at_stat;
    }
}

void cpu_task::config(const task_config_t&)
{
    OP_LOG(OP_LOG_INFO, "Set config\n");
}

task_config_t cpu_task::config() const
{
    OP_LOG(OP_LOG_INFO, "Get config\n");
    return {};
}

task_stat_t cpu_task::stat() const
{
    return *m_at_stat;
}

void cpu_task::clear_stat()
{
    *m_at_stat = {};
    m_reset_stat = true;
}

}