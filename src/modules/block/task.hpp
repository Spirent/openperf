#ifndef _OP_BLOCK_GENERATOR_TASK_HPP_
#define _OP_BLOCK_GENERATOR_TASK_HPP_

#include <atomic>
#include <chrono>

#include <aio.h>

#include "pattern_generator.hpp"

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"

namespace openperf::block::worker {

using ref_clock = timesync::chrono::monotime;
using realtime = timesync::chrono::realtime;
using duration = std::chrono::nanoseconds;

enum class task_operation : uint8_t { READ = 0, WRITE };

struct task_synchronizer
{
    std::atomic_int32_t ratio_reads;
    std::atomic_int32_t ratio_writes;
    std::atomic_int64_t reads_actual = 0;
    std::atomic_int64_t writes_actual = 0;
};

struct task_config_t
{
    int fd;
    size_t f_size;
    size_t header_size;
    size_t queue_depth;
    task_operation operation;
    uint32_t ops_per_sec;
    size_t block_size;
    model::block_generation_pattern pattern;
    task_synchronizer* synchronizer = nullptr;
};

struct task_stat_t
{
    using optional_time_t = std::optional<duration>;
    /**
     * updated      - Date of the last result update
     * ops_target   - The intended number of operations performed
     * ops_actual   - The actual number of operations performed
     * bytes_target - The intended number of bytes read or written
     * bytes_actual - The actual number of bytes read or written
     * errors       - The number of io_errors observed during reading or writing
     * latency      - The total amount of time required to perform operations
     * latency_min  - The minimum observed latency value
     * latency_max  - The maximum observed latency value
     */

    task_operation operation;
    realtime::time_point updated = realtime::now();
    uint_fast64_t ops_target = 0;
    uint_fast64_t ops_actual = 0;
    uint_fast64_t bytes_target = 0;
    uint_fast64_t bytes_actual = 0;
    uint_fast64_t errors = 0;
    duration latency = duration::zero();
    optional_time_t latency_min;
    optional_time_t latency_max;

    task_stat_t& operator+=(const task_stat_t&);
};

class block_task : public framework::generator::task<task_stat_t>
{
    enum aio_state : uint8_t {
        IDLE = 0,
        PENDING,
        COMPLETE,
        FAILED,
    };

    struct operation_state
    {
        ref_clock::time_point start;
        ref_clock::time_point stop;
        uint64_t io_bytes;
        enum aio_state state;
        aiocb aiocb;
    };

    struct operation_config;

private:
    task_config_t m_task_config;
    task_stat_t m_stat;
    std::vector<operation_state> m_aio_ops;
    std::vector<uint8_t> m_buf;
    pattern_generator m_pattern;
    ref_clock::time_point m_operation_timestamp;

public:
    block_task(const task_config_t&);

    task_config_t config() const { return m_task_config; }

    task_stat_t spin() override;
    void reset() override;

private:
    void config(const task_config_t&);
    void reset_spin_stat();
    int32_t calculate_rate();
    task_stat_t worker_spin(uint64_t nb_ops, ref_clock::time_point deadline);

private:
    static int complete_aio_op(struct operation_state& aio_op);
    static int wait_for_aio_ops(std::vector<operation_state>& aio_ops,
                                size_t nb_ops);
    static int submit_aio_op(const operation_config& op_config,
                             operation_state& op_state);
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_
