#ifndef _OP_BLOCK_GENERATOR_WORKER_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "block/worker_task.hpp"
#include "core/op_core.h"

namespace openperf::block::worker {

struct worker_config {
    int32_t queue_depth;
    int32_t reads_per_sec;
    int32_t read_size;
    int32_t writes_per_sec;
    int32_t write_size;
    std::string pattern;
};

const std::string endpoint_prefix = "inproc://openperf_block_worker";

typedef std::unique_ptr<worker_task> worker_task_ptr;
typedef std::vector<worker_task_ptr> worker_task_vec;

class block_worker {
private:
    worker_task_vec tasks;
    bool running;
    worker_config config;
    void* m_context;
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<std::thread> worker_tread;

    void worker_loop(void* context);
public:
    block_worker(const worker_config& p_config, bool p_running);
    ~block_worker();

    template<typename T>
    void add_task(T* task)
    {
        tasks.push_back(std::unique_ptr<T>(task));
    };
    void set_running(bool p_running);
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_