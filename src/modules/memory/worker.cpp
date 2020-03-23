#include "memory/worker.hpp"

#include <zmq.h>
#include <iostream>

namespace openperf::memory::internal {

// Constructors & Destructor
worker::worker()
    : _zmq_context(zmq_ctx_new())
    , _zmq_socket(op_socket_get_server(_zmq_context, ZMQ_PAIR, endpoint_prefix))
{
    _state.paused = true;
    _state.stopped = true;
    _state.buffer_size = 16;
    _state.block_size = 1;
    _state.op_per_sec = 1;
}

worker::worker(const worker::config& config)
    : worker()
{
    set_config(config);
}

worker::worker(worker&& worker)
    : _zmq_context(std::move(worker._zmq_context))
    , _zmq_socket(std::move(worker._zmq_socket))
    , _thread(std::move(worker._thread))
    , _tasks(std::move(worker._tasks))
    , _state(worker._state)
{}

worker::~worker()
{
    std::cout << "worker::~worker()" << std::endl;
    if (_thread.joinable()) {
        _thread.detach();
        _state.stopped = true;
        _state.paused = false;
        update();
    }

    zmq_close(_zmq_socket.get());
    zmq_ctx_shutdown(_zmq_context);
    zmq_ctx_term(_zmq_context);
}

// Methods : public
void worker::add_task(std::unique_ptr<task> task)
{
    _tasks.emplace_front(std::move(task));
}

void worker::start()
{
    if (!_state.stopped) return;
    _state.stopped = false;

    std::cout << "Start worker " << _state.stopped << " " << _state.paused
              << std::endl;
    _thread = std::thread([this]() { loop(); });
    update();
}

void worker::stop()
{
    if (_state.stopped) return;
    _state.stopped = true;
    _thread.detach();
    update();
}

void worker::pause()
{
    if (_state.paused) return;
    _state.paused = true;
    update();
}

void worker::resume()
{
    if (!_state.paused) return;
    _state.paused = false;
    update();
}

void worker::set_config(const worker::config& c)
{
    if (!c.stopped) start();
    _state = c;
    update();
}

// Methods : private
void worker::loop()
{
    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " started" << std::endl;
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_zmq_context, ZMQ_PAIR, endpoint_prefix));

    worker::config msg{.paused = true, .stopped = false};

    for (;;) {
        int recv = zmq_recv(
            socket.get(), &msg, sizeof(msg), msg.paused ? 0 : ZMQ_NOBLOCK);
        if (recv < 0 && errno != EAGAIN) {
            std::cerr << "Shutdown" << std::endl;
            break;
        }

        std::cout << "Thread " << std::this_thread::get_id()
                  << " pause: " << msg.paused << ", stop: " << msg.stopped
                  << std::endl;

        if (msg.stopped) {
            std::cout << "Gracesfully shutdown" << std::endl;
            break;
        }

        if (!msg.paused) {
            for (auto& task : _tasks) { task->run(); }
        }
    }

    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " finished" << std::endl;
}

void worker::update()
{
    if (is_finished()) return;
    zmq_send(_zmq_socket.get(), &_state, sizeof(_state), 0); // ZMQ_NOBLOCK);
    std::cout << "worker::update()" << std::endl;
}

} // namespace openperf::memory::generator
