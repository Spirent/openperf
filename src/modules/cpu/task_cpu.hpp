#ifndef _OP_CPU_TASK_CPU_HPP_
#define _OP_CPU_TASK_CPU_HPP_

#include <atomic>
#include <forward_list>
#include <memory>

#include "utils/worker/task.hpp"
#include "cpu/target.hpp"
#include "cpu/common.hpp"
#include "cpu/task_cpu_stat.hpp"

namespace openperf::cpu::internal {

struct target_config {
    cpu::instruction_set set;
    cpu::data_size data_size;
    cpu::operation operation;
    uint64_t weight;
};

struct task_cpu_config {
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

class task_cpu :
    public utils::worker::task<task_cpu_config, task_cpu_stat>
{
    using target_ptr = std::unique_ptr<target>;

    struct target_meta {
        uint64_t weight;
        uint64_t runtime;
        target_ptr target;
    };

private:
    config_t m_config;
    stat_t m_stat_shared, m_stat_active;

    std::atomic<stat_t*> m_stat;
    std::atomic_bool m_stat_clear;

    uint64_t m_weights;
    std::forward_list<target_meta> m_targets;
    uint64_t m_targets_count;
    double m_utilization;

public:
    task_cpu();
    explicit task_cpu(const config_t&);
    ~task_cpu() override = default;

    void spin() override;

    void config(const config_t&) override;
    inline config_t config() const override { return m_config; }

    stat_t stat() const override;
    inline void clear_stat() override { m_stat_clear = true; }
};

} // namespace openperf::cpu::internal


#endif // _OP_CPU_TASK_CPU_HPP_
