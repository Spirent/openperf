#ifndef _OP_BLOCK_GENERATOR_TASK_HPP_
#define _OP_BLOCK_GENERATOR_TASK_HPP_

#include <atomic>
#include <chrono>

#include <aio.h>

#include "pattern_generator.hpp"

#include "framework/utils/worker/task.hpp"
#include "modules/timesync/chrono.hpp"

namespace openperf::block::worker {

using ref_clock = timesync::chrono::monotime;
using realtime = timesync::chrono::realtime;
using time_point = realtime::time_point;
using duration = std::chrono::nanoseconds;

enum task_operation { READ = 0, WRITE };

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
    int32_t ops_per_sec;
    size_t block_size;
    model::block_generation_pattern pattern;
    task_synchronizer* synchronizer = nullptr;
};

struct task_stat_t
{
    time_point updated = realtime::now(); /* Date of the last result update */
    uint_fast64_t ops_target =
        0; /* The intended number of operations performed */
    uint_fast64_t ops_actual =
        0; /* The actual number of operations performed */
    uint_fast64_t bytes_target =
        0; /* The intended number of bytes read or written */
    uint_fast64_t bytes_actual =
        0; /* The actual number of bytes read or written */
    uint_fast64_t errors =
        0; /* The number of io_errors observed during reading or writing */
    duration latency = duration::zero(); /* The total amount of time required to
                                            perform all operations */
    duration latency_min =
        duration::max(); /* The minimum observed latency value */
    duration latency_max =
        duration::zero(); /* The maximum observed latency value */
};

enum aio_state {
    IDLE = 0,
    PENDING,
    COMPLETE,
    FAILED,
};

struct operation_state
{
    time_point start;
    time_point stop;
    uint64_t io_bytes;
    enum aio_state state;
    aiocb aiocb;
};

class block_task : public utils::worker::task<task_config_t, task_stat_t>
{
private:
    task_config_t m_task_config;
    task_stat_t m_actual_stat, m_shared_stat; // Switchable actual statistics
    std::atomic<task_stat_t*> m_at_stat;      // Statistics to return
    std::atomic_bool m_reset_stat; // True if statistics have to be reseted
    std::vector<operation_state> m_aio_ops;
    std::vector<uint8_t> m_buf;
    pattern_generator m_pattern;
    time_point m_operation_timestamp, m_pause_timestamp, m_start_timestamp;

    size_t worker_spin(task_config_t& op_config,
                       task_stat_t& op_stat,
                       uint64_t nb_ops,
                       time_point deadline);
    void reset_spin_stat();
    int32_t calculate_rate();

public:
    block_task();
    ~block_task();

    void spin() override;
    void resume() override;
    void pause() override;
    void config(const task_config_t&) override;
    task_config_t config() const override;
    task_stat_t stat() const override;
    void clear_stat() override;
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_