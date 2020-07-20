#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <vector>
#include <chrono>

#include "framework/generator/task.hpp"
#include "timesync/chrono.hpp"
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
    std::vector<uint64_t>* indexes = nullptr;
};

class task_memory : public openperf::framework::generator::task<memory_stat>
{
    using chronometer = openperf::timesync::chrono::monotime;

protected:
    task_memory_config m_config;
    uint8_t* m_buffer = nullptr;

    struct
    {
        void* ptr;
        size_t size;
    } m_scratch;

    task_memory_stat m_stat;
    size_t m_op_index;
    double m_avg_rate;

public:
    task_memory() = delete;
    task_memory(task_memory&&) noexcept;
    task_memory(const task_memory&) = delete;
    explicit task_memory(const task_memory_config&);
    ~task_memory() override;

    void reset() override;

    task_memory_config config() const { return m_config; }
    memory_stat spin() override;

protected:
    void config(const task_memory_config&);
    virtual void operation(uint64_t nb_ops) = 0;
    virtual memory_stat make_stat(const task_memory_stat&) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
