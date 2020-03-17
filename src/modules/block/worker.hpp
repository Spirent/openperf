#ifndef _OP_BLOCK_GENERATOR_WORKER_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "block/worker_task.hpp"
#include "block/models/generator.hpp"
#include "core/op_core.h"

namespace openperf::block::worker {

constexpr auto endpoint_prefix = "inproc://openperf_block_worker";

typedef std::unique_ptr<worker_task> worker_task_ptr;
typedef std::vector<worker_task_ptr> worker_task_vec;

struct worker_config {
    int32_t queue_depth;
    int32_t reads_per_sec;
    int32_t read_size;
    int32_t writes_per_sec;
    int32_t write_size;
};

struct worker_state {
    worker_config config;
    bool running;
    int fd;
};

class block_worker {
private:
    worker_task_vec tasks;
    worker_state state;
    void* m_context;
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<std::thread> worker_tread;
    
    void worker_loop(void* context);
    template<typename T>
    void add_task(T* task);
    void clean_tasks();
    void update_state();

public:
    block_worker(const worker_config& p_config, int fd, bool p_running, const model::block_generation_pattern& pattern);
    ~block_worker();

    void set_running(bool p_running);
    bool is_running() const;
    void set_resource_descriptor(int fd);
    void set_pattern(const model::block_generation_pattern& pattern);
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_