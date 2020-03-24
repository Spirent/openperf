#ifndef _OP_BLOCK_GENERATOR_WORKER_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "worker_pattern.hpp"

namespace openperf::block::worker {

typedef model::block_generation_pattern worker_pattern;

struct worker_config {
    size_t f_size;
    int32_t queue_depth;
    int32_t reads_per_sec;
    size_t read_size;
    int32_t writes_per_sec;
    size_t write_size;
};

class block_worker {
private:
    struct {
        worker_config config;
        bool running;
        int fd;
        worker_pattern pattern;
    } state;
    void *m_context;
    void *m_socket;
    std::unique_ptr<std::thread> worker_tread;

    void worker_loop(void* context);
    void update_state();

public:
    block_worker(const worker_config& p_config, int fd, bool p_running, const worker_pattern& pattern);
    ~block_worker();

    void set_running(bool p_running);
    bool is_running() const;
    void set_resource_descriptor(int fd);
    void set_pattern(const model::block_generation_pattern& pattern);
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_