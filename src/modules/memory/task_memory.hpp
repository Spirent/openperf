#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <cstddef>
#include <atomic>
#include <vector>
#include <limits>

#include "utils/worker/task.hpp"
#include "memory/io_pattern.hpp"

namespace openperf::memory::internal {

struct task_memory_config
{
    size_t block_size;
    size_t buffer_size;
    size_t op_per_sec;
    io_pattern pattern;
};

struct memory_stat
{
    uint64_t operations = 0; //< The number of operations performed
    uint64_t operations_target = 0;
    uint64_t bytes = 0; //< The number of bytes read or written
    uint64_t bytes_target = 0;
    uint64_t errors = 0; //< The number of errors during reading or writing
    uint64_t time_ns = 0; //< The number of ns required for the operations
    uint64_t latency_max = 0;
    uint64_t latency_min = std::numeric_limits<uint64_t>::max();
    uint64_t timestamp = 0;
};

class task_memory :
    public openperf::utils::worker::task<task_memory_config, memory_stat>
{
protected:
    task_memory_config _config;
    uint64_t _cache_size = 16;
    uint8_t* _buffer;
    std::vector<unsigned> _indexes;
    size_t _op_index_min;
    size_t _op_index_max;

    void* _scratch_buffer;
    size_t _scratch_size;

    struct total
    {
        size_t operations = 0;
        size_t run_time = 0;
        size_t sleep_time = 0;
        double avg_rate = 100000000;
    } _total;

    std::atomic<stat_t> _stat;
    std::atomic_bool _stat_clear;

public:
    task_memory();

    void spin() override;
    void config(const config_t&) override;
    void clear_stat() override;

    inline config_t config() const override { return _config; }
    inline stat_t stat() const override { return _stat.load(); }

protected:
    virtual size_t operation(uint64_t nb_ops, size_t* op_idx) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
