#include "worker.hpp"
#include "core/op_core.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"

#include <zmq.h>
#include <iostream>

namespace openperf::memory::generator {

namespace model = swagger::v1::model;

void loop_function(void* context);

worker::worker() : 
    _running(false), _context(zmq_ctx_new()), _thread(loop_function, _context)
{
}

worker::~worker()
{
    if (_thread.joinable()) _thread.detach();
    zmq_ctx_shutdown(_context);
}

void worker::add_task(task_base &task)
{
    _tasks.push_front(task);
}

void worker::start()
{
    if (_running) return;

    if (_thread.joinable())
        _thread.detach();
    _thread = std::thread([this](){ loop(); });
}

void worker::stop()
{
}

void worker::loop()
{
    std::cout << "Thread " << std::hex << std::this_thread::get_id() << " started" << std::endl;
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_context, ZMQ_REP, endpoint_prefix));

    bool running = false;
    model::MemoryGeneratorConfig config;

    for (;;) {
        int recv = zmq_recv(socket.get(), &config, sizeof(config), running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            std::cerr << "Shutdown" << std::endl;
            break;
        }

        std::cout << "Thread work" << std::endl; 
        if (running) {
            for (auto& task : _tasks) {
                task.get().run();
            }
        }
    }

    std::cout << "Thread " << std::hex << std::this_thread::get_id() << " finished" << std::endl;
}

void loop_function(void *context) {
    std::cout << "Thread " << std::hex << std::this_thread::get_id() << " started" << std::endl;
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(context, ZMQ_REP, endpoint_prefix));

    bool running = false;
    model::MemoryGeneratorConfig config;
    
    for (;;) {
        int recv = zmq_recv(socket.get(), &config, sizeof(config), running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            std::cerr << "Shutdown" << std::endl;
            break;
        }

        std::cout << "Thread work" << std::endl; 
        if (running) {
            //for (auto& task : _tasks) {
            //    task.get().run();
            //}
        }
    }

    std::cout << "Thread " << std::hex << std::this_thread::get_id() << " finished" << std::endl;
}

} // namespace openperf::memory::worker
