#ifndef _OP_BLOCK_GENERATOR_TASK_HPP_
#define _OP_BLOCK_GENERATOR_TASK_HPP_

#include <aio.h>
#include "pattern_generator.hpp"
#include "utils/worker/task.hpp"

namespace openperf::block::worker {

typedef model::block_generation_pattern worker_pattern;

struct task_config_t {
    int fd;
    size_t f_size;
    int32_t queue_depth;
    int32_t reads_per_sec;
    size_t read_size;
    int32_t writes_per_sec;
    size_t write_size;
    worker_pattern pattern;
};

enum aio_state {
    IDLE = 0,
    PENDING,
    COMPLETE,
    FAILED,
};

struct operation_state {
    uint64_t start_ns;
    uint64_t stop_ns;
    uint64_t io_bytes;
    enum aio_state state;
    aiocb aiocb;
};

class block_task : public utils::worker::task<task_config_t> {
private:
    task_config_t config;
    operation_state *aio_ops;
    uint8_t* buf;
    pattern_generator pattern;
    int64_t read_timestamp, write_timestamp;

    size_t worker_spin(int (*queue_aio_op)(aiocb *aiocb));

public:
    block_task();
    ~block_task();

    void run();
    void set_config(const task_config_t&);
    task_config_t get_config() const;
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_