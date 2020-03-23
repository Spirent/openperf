#ifndef _OP_MEMORY_TASK_MEMORY_HPP_
#define _OP_MEMORY_TASK_MEMORY_HPP_

#include <cstddef>
#include <atomic>
#include <vector>

#include "memory/task.hpp"
#include "memory/io_pattern.hpp"

namespace openperf::memory::internal {

class task_memory 
    : public openperf::generator::generic::task
{
public:
    struct config_msg
    {
        size_t block_size;
        size_t buffer_size;
        size_t op_per_sec;
        io_pattern pattern;
        // struct {
        //    uint8_t *ptr;
        //    size_t size;
        //} buffer;
        // struct {
        //    void (*fn)(void *, uint64_t);
        //    void *arg;
        //} callback;
    };

protected:
    uint64_t _cache_size = 16;
    io_pattern _pattern;
    size_t _block_size;
    size_t _op_per_sec;
    size_t _buffer_size;

    struct worker_config
    {
        // struct op_packed_array *indexes;
        std::vector<unsigned> indexes;
        uint8_t* buffer;
        size_t op_index_min;
        size_t op_index_max;

        size_t op_block_size;
        size_t op_per_sec;
        enum io_pattern pattern;
    } _config;

    // struct scratch_area {
    //    void* buffer
    //    size_t size;
    //} _scratch;

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
            time_ns; /**< The number of ns required for the operations */
        std::atomic_uint_fast64_t
            operations; /**< The number of operations performed */
        std::atomic_uint_fast64_t
            bytes; /**< The number of bytes read or written */
        std::atomic_uint_fast64_t
            errors; /**< The number of errors during reading or writing */
    } _stats;

public:
    task_memory();

    void run() override;
    int set_config(const config_msg&);

    void set_buffer_size(size_t);
    void set_block_size(size_t);
    void set_rate(size_t);
    void set_pattern(io_pattern);

    size_t buffer_size() const { return _buffer_size; }
    size_t block_size() const { return _block_size; }
    size_t rate() const { return _op_per_sec; }
    io_pattern pattern() const { return _pattern; }

    uint64_t time_ns() const { return _stats.time_ns; }
    uint64_t operations() const { return _stats.operations; }
    uint64_t bytes() const { return _stats.bytes; }
    uint64_t errors() const { return _stats.errors; }

protected:
    virtual size_t spin(uint64_t nb_ops, size_t* op_idx) = 0;

private:
    void scratch_allocate(size_t size);
    void scratch_free();
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_HPP_
