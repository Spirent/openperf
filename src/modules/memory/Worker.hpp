#ifndef _OP_MEMORY_GENERATOR_WORKER_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_HPP_

#include <thread>
#include <forward_list>

#include "memory/TaskBase.hpp"
#include "core/op_core.h"

namespace openperf::memory::generator {

class Worker
{
public:
    struct Config
    {
        bool paused = false;
        bool stopped = true;
        unsigned int buffer_size = 8;
        unsigned int op_per_sec = 1;
        unsigned int block_size = 1;
    };

private:
    struct Message
    {
        bool paused;
        bool stopped;
        unsigned int buffer_size;
        unsigned int op_per_sec;
        unsigned int block_size;
    };

private:
    void* _zmq_context;
    std::unique_ptr<void, op_socket_deleter> _zmq_socket;
    std::thread _thread;
    std::forward_list<std::unique_ptr<TaskBase>> _tasks;
    //bool _paused;
    //bool _stopped;
    //unsigned int _buffer_size;
    //unsigned int _op_per_sec;
    //unsigned int _block_size;
    Config _state;

    static constexpr auto endpoint_prefix = "inproc://openperf_memory_Worker";

public:
    Worker();
    Worker(Worker&&);
    Worker(const Worker&) = delete;
    Worker(const Worker::Config&);
    ~Worker();
    
    void start();
    void stop();
    void pause();
    void resume();

    void addTask(std::unique_ptr<TaskBase>);

    void setConfig(const Worker::Config&);

    inline bool isPaused() const { return _state.paused; }
    inline bool isRunning() const { return !isPaused(); }
    inline bool isFinished() const { return _state.stopped; }
    inline int bufferSize() const { return _state.block_size; }
    inline int opPerSec() const { return _state.op_per_sec; }
    inline int blockSize() const { return _state.block_size; }

private:
    void loop();
    void update();
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_WORKER_HPP_
