#ifndef _OP_CPU_TASK_CPU_HPP_
#define _OP_CPU_TASK_CPU_HPP_

#include <cinttypes>
#include <atomic>
#include <vector>
#include <forward_list>
#include <memory>

#include "utils/worker/task.hpp"
#include "cpu/target.hpp"

namespace openperf::cpu::internal
{

enum class instruction_set {
    SCALAR = 1
};

struct target_config {
    instruction_set set;
    target::data_size_t data_size;
    target::operation_t operation;
    uint32_t weight;
};

struct task_cpu_config {
    std::vector<target_config> targets;
};

struct target_cpu_stat {
    uint64_t cycles;
};

struct task_cpu_stat {
    uint64_t available = 0;
    uint64_t utilization = 0;
    uint64_t system = 0;
    uint64_t user = 0;
    uint64_t steal = 0;
    uint64_t error = 0;
    std::vector<target_cpu_stat> targets;

    task_cpu_stat() = default;

    task_cpu_stat(const task_cpu_stat& other) {
        copy(other);
    }

    task_cpu_stat& operator= (const task_cpu_stat& other) {
        copy(other);
        return *this;
    }

private:
    void copy (const task_cpu_stat& other) {
        available = other.available;
        utilization = other.utilization;
        system = other.system;
        user = other.user;
        steal = other.steal;
        error = other.error;

        targets.resize(other.targets.size());
        for (size_t i = 0; i < other.targets.size(); ++i)
            targets[i] = other.targets[i];
    }
};

class task_cpu :
    public utils::worker::task<task_cpu_config, task_cpu_stat>
{
    using target_ptr = std::unique_ptr<target>;

private:
    config_t m_config;
    stat_t m_stat_shared, m_stat_active;

    std::atomic<stat_t*> m_stat;
    std::atomic_bool m_stat_clear;

    std::forward_list<target_ptr> m_targets;

    void update_shared_stat();

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