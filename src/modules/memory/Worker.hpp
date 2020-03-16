#ifndef _OP_MEMORY_GENERATOR_WORKER_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_HPP_

#include <thread>
#include <list>
#include <atomic>
#include "TaskBase.hpp"
#include "core/op_core.h"

namespace openperf::memory::generator {

class Worker
{
private:
    typedef std::unique_ptr<void, op_socket_deleter> ZmqSocketPointer;

    struct ZmqMessage
    {
        bool running;
        bool stopping;
    };

private:
    // std::atomic_bool _stopFlag;
    bool _paused;
    bool _stopped;
    void* _context;
    std::thread _thread;
    TaskList _tasks;
    ZmqSocketPointer _zmqSocket;

    static constexpr auto endpoint_prefix = "inproc://openperf_memory_Worker";

public:
    Worker();
    Worker(const Worker&) = delete;
    ~Worker();

    void addTask(std::unique_ptr<TaskBase>);

    // bool is_running() const;
    void start();
    void stop();
    void pause();
    void resume();

private:
    void loop();
    void update();
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_WORKER_HPP_
