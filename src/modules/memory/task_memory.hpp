#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <atomic>
#include <vector>

#include "utils/worker/task.hpp"
#include "memory/io_pattern.hpp"
#include "memory/memory_stat.h"

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
protected:
    task_memory_config _config;
    std::vector<unsigned> _indexes;
    size_t _op_index_min;
    size_t _op_index_max;
    uint8_t* _buffer;

    struct
    {
        void* ptr;
        size_t size;
    } _scratch;

    struct total
    {
        size_t operations = 0;
        size_t run_time = 0;
        size_t sleep_time = 0;
        double avg_rate = 100000000;
    } _total;

    stat_t _stat_data;
    std::atomic<stat_t*> _stat;
    std::atomic_bool _stat_clear;

public:
    task_memory();
    explicit task_memory(const task_memory_config&);
    ~task_memory();

    void spin() override;
    void config(const config_t&) override;
    inline void clear_stat() override { _stat_clear = true; };

    stat_t stat() const override;
    inline config_t config() const override { return _config; }

protected:
    virtual size_t operation(uint64_t nb_ops, size_t* op_idx) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
