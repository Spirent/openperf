#ifndef _OP_MEMORY_GENERATOR_WORKER_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_HPP_

#include <thread>
#include <list>
#include "modules/memory/task_base.hpp"

namespace openperf::memory::generator {

constexpr auto endpoint_prefix = "inproc://openperf_memory_worker";

class worker {
private:
    typedef std::list< std::reference_wrapper<task_base> > task_list;

private:
    volatile bool  _running;
    void*  _context;
    std::thread  _thread;    
    task_list  _tasks;

public:
    worker();
    ~worker();

    void add_task(task_base&);

    //bool is_running() const;
    void start();
    void stop();

private:
    void loop();
};

} // namespace openperf::memory::worker

#endif // _OP_MEMORY_GENERATOR_WORKER_HPP_
