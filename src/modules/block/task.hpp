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
    uint_fast64_t ops_target; /* The intended number of operations performed */
    uint_fast64_t ops_actual; /* The actual number of operations performed */
    uint_fast64_t
        bytes_target; /* The intended number of bytes read or written */
    uint_fast64_t bytes_actual; /* The actual number of bytes read or written */
    uint_fast64_t
        errors; /* The number of io_errors observed during reading or writing */
    uint_fast64_t latency; /* The total amount of time required to perform all
                              operations */
    uint_fast64_t latency_min; /* The minimum observed latency value */
    uint_fast64_t latency_max; /* The maximum observed latency value */
};

struct task_stat_t
{
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
    uint64_t start_ns;
    uint64_t stop_ns;
    uint64_t io_bytes;
    enum aio_state state;
    aiocb aiocb;
};

class block_task : public utils::worker::task<task_config_t, task_stat_t>
{
private:
    task_config_t task_config;
    task_stat_t actual_stat, shared_stat; // Switchable actual statistics
    std::atomic<task_stat_t*> at_stat;    // Statistics to return
    std::atomic_bool reset_stat; // True if statistics have to be reseted
    std::vector<operation_state> aio_ops;
    std::vector<uint8_t> buf;
    pattern_generator pattern;
    time_point read_timestamp, write_timestamp, pause_timestamp,
        start_timestamp;

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