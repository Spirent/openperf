#ifndef _OP_BLOCK_GENERATOR_TASK_HPP_
#define _OP_BLOCK_GENERATOR_TASK_HPP_

#include <aio.h>
#include <atomic>
#include <chrono>
#include "pattern_generator.hpp"
#include "utils/worker/task.hpp"
#include "timesync/chrono.hpp"

namespace openperf::block::worker {

using worker_pattern = model::block_generation_pattern;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;
using ref_clock = timesync::chrono::monotime;
using duration = std::chrono::duration<uint64_t, std::nano>;
struct task_config_t
{
    int fd;
    size_t f_size;
    size_t header_size;
    size_t queue_depth;
    int32_t reads_per_sec;
    size_t read_size;
    int32_t writes_per_sec;
    size_t write_size;
    worker_pattern pattern;
};

struct task_operation_stat_t
{
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

struct task_stat_t
{
    std::string id;
    time_point updated; /* Date of the last result update */
    task_operation_stat_t read;
    task_operation_stat_t write;
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
    time_point m_read_timestamp, m_write_timestamp, m_pause_timestamp,
        m_start_timestamp;

    size_t worker_spin(int (*queue_aio_op)(aiocb* aiocb),
                       size_t block_size,
                       task_operation_stat_t& op_stat,
                       time_point deadline);

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