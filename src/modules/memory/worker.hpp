#ifndef _OP_MEMORY_GENERATOR_WORKER_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_HPP_

#include <thread>
#include <forward_list>

#include "core/op_core.h"
#include "memory/task.hpp"

namespace openperf::memory::internal {

using openperf::generator::generic::task;

class worker
{
public:
    struct config
    {
        bool paused = false;
        bool stopped = true;
        unsigned int buffer_size = 8;
        unsigned int op_per_sec = 1;
        unsigned int block_size = 1;
    };

private:
    struct message
     {
        bool paused;
        bool stopped;
        unsigned int buffer_size;
        unsigned int op_per_sec;
        unsigned int block_size;
    };

    static constexpr auto endpoint_prefix = "inproc://openperf_memory_worker";

private:
    void* _zmq_context;
    std::unique_ptr<void, op_socket_deleter> _zmq_socket;
    std::thread _thread;
    std::forward_list<std::unique_ptr<task>> _tasks;
    // bool _paused;
    // bool _stopped;
    // unsigned int _buffer_size;
    // unsigned int _op_per_sec;
    // unsigned int _block_size;
    config _state;

public:
    worker();
    worker(worker&&);
    worker(const worker&) = delete;
    worker(const worker::config&);
    ~worker();

    void start();
    void stop();
    void pause();
    void resume();

    void add_task(std::unique_ptr<task>);

    void set_config(const worker::config&);

    inline bool is_paused() const { return _state.paused; }
    inline bool is_running() const { return !is_paused(); }
    inline bool is_finished() const { return _state.stopped; }
    inline int buffer_size() const { return _state.block_size; }
    inline int op_per_sec() const { return _state.op_per_sec; }
    inline int block_size() const { return _state.block_size; }

private:
    void loop();
    void update();
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_WORKER_HPP_
