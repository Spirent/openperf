#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <atomic>
#include <vector>
#include <chrono>

#include "timesync/chrono.hpp"
#include "utils/worker/task.hpp"
#include "memory/io_pattern.hpp"
#include "memory/memory_stat.hpp"

namespace openperf::memory::internal {

struct task_memory_config
{
    size_t block_size = 0;
    size_t op_per_sec = 0;
    io_pattern pattern = io_pattern::NONE;
    struct
    {
        void* ptr = nullptr;
        size_t size = 0;
    } buffer;
};

class task_memory
    : public openperf::utils::worker::task<task_memory_config, memory_stat>
{
    using chronometer = openperf::timesync::chrono::monotime;

protected:
    task_memory_config m_config;
    std::vector<unsigned> m_indexes;
    uint8_t* m_buffer = nullptr;

    struct
    {
        void* ptr;
        size_t size;
    } m_scratch;

    stat_t m_stat_data;
    std::atomic<stat_t*> m_stat;
    std::atomic_bool m_stat_clear;
    size_t m_op_index;
    double m_avg_rate;

public:
    task_memory();
    explicit task_memory(const task_memory_config&);
    ~task_memory() override;

    void spin() override;
    void config(const config_t&) override;
    void clear_stat() override { m_stat_clear = true; };

    stat_t stat() const override;
    config_t config() const override { return m_config; }

protected:
    virtual size_t operation(uint64_t nb_ops) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
