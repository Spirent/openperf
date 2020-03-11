#include "worker.hpp"
#include "core/op_core.h"

namespace openperf::block::worker 
{

block_worker::block_worker():
    done(false)
{
    auto p_thread = new std::thread([this]() {
        worker_loop();
    });
    worker_tread = std::unique_ptr<std::thread>(p_thread);
}

block_worker::~block_worker() 
{
    done = true;
    worker_tread -> join();
    tasks.clear();
}

template<typename T>
void block_worker::add_task(T &task) 
{
    tasks.push_back(std::make_unique<T>(task));
}

void block_worker::start() 
{
    std::unique_lock<decltype(m)> l(m);
    running = true;
}

void block_worker::stop() 
{
    std::unique_lock<decltype(m)> l(m);
    running = false;
}

void block_worker::worker_loop()
{
    for (;!done;)
    {
        std::unique_lock<decltype(m)> l(m);
        //cv.wait(l, [this]{ return running; });
        printf("Still working\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    printf("DONE\n");
}

} // namespace openperf::block::worker