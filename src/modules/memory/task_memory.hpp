#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <cstddef>
#include <atomic>
#include <vector>

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

class task_memory : public openperf::utils::worker::task<task_memory_config>
{
protected:
    task_memory_config _config;
    uint64_t _cache_size = 16;
    uint8_t* _buffer;
    // struct op_packed_array *indexes;
    std::vector<unsigned> _indexes;
    size_t _op_index_min;
    size_t _op_index_max;
    // io_pattern _pattern;
    // size_t _block_size;
    // size_t _op_per_sec;
    // size_t _buffer_size;

    // struct
    //{
    //    size_t op_block_size;
    //    size_t op_per_sec;
    //    enum io_pattern pattern;
    //}

    void* _scratch_buffer;
    size_t _scratch_size;

    struct total
    {
        size_t operations = 0;
        size_t run_time = 0;
        size_t sleep_time = 0;
        double avg_rate = 100000000;
    } _total;

    /**
     * Structure describing the statistics collected from worker threads
     */
    struct stats
    {
        std::atomic_uint_fast64_t
            time_ns = 0; /**< The number of ns required for the operations */
        std::atomic_uint_fast64_t
            operations = 0; /**< The number of operations performed */
        std::atomic_uint_fast64_t
            bytes = 0; /**< The number of bytes read or written */
        std::atomic_uint_fast64_t
            errors = 0; /**< The number of errors during reading or writing */
    } _stats;

public:
    task_memory();

    void spin() override;
    void config(const task_memory_config&) override;

    task_memory_config config() const override { return _config; }

protected:
    virtual size_t operation(uint64_t nb_ops, size_t* op_idx) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
