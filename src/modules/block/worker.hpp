#ifndef _OP_BLOCK_GENERATOR_WORKER_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "block/worker_task.hpp"

namespace openperf::block::worker {

typedef std::unique_ptr<worker_task> worker_task_ptr;
typedef std::vector<worker_task_ptr> worker_task_vec;

class block_worker {
private:
    worker_task_vec tasks;
    bool running;
    bool done;
    std::unique_ptr<std::thread> worker_tread;
    std::mutex m;
    std::condition_variable cv;
    void worker_loop();
public:
    block_worker();
    ~block_worker();

    template<typename T>
    void add_task(T &);
    void start();
    void stop();
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_HPP_