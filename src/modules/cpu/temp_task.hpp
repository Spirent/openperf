#ifndef _OP_CPU_GENERATOR_TASK_HPP_
#define _OP_CPU_GENERATOR_TASK_HPP_

#include "utils/worker/task.hpp"
#include <atomic>

namespace openperf::cpu::worker {

struct task_config_t{};

struct task_stat_t{
    uint64_t cycles;
};

class cpu_task : public utils::worker::task<task_config_t, task_stat_t>
{
private:
    task_stat_t m_actual_stat, m_shared_stat; // Switchable actual statistics
    std::atomic<task_stat_t*> m_at_stat;      // Statistics to return
    std::atomic_bool m_reset_stat; // True if statistics have to be reseted

public:
    cpu_task();
    ~cpu_task();

    void spin() override;
    void config(const task_config_t&) override;
    task_config_t config() const override;
    task_stat_t stat() const override;
    void clear_stat() override;
};


}

#endif // _OP_CPU_GENERATOR_TASK_HPP_