#ifndef _OP_MEMORY_GENERATOR_WORKER_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_HPP_

#include <thread>
#include <forward_list>
#include <zmq.h>
#include <iostream>

#include "core/op_core.h"
#include "memory/task.hpp"
#include "memory/worker_interface.hpp"

namespace openperf::memory::internal {

using openperf::generator::generic::task;

template <class T>
class worker : public worker_interface
{
    //static_assert(std::is_base_of<task, T>::value);
    template <class U>
    friend class worker;

private:
    struct message
    {
        bool paused;
        bool stopped;
        typename T::config_t config;
    };

    static constexpr auto endpoint_prefix = "inproc://openperf_memory_worker";

private:
    bool _paused;
    bool _stopped;
    void* _zmq_context;
    std::unique_ptr<void, op_socket_deleter> _zmq_socket;
    std::thread _thread;
    std::unique_ptr<T> _task;
    message _msg;

public:
    worker();
    worker(worker&&);
    worker(const worker&) = delete;
    ~worker();

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    
    inline bool is_paused() const override { return _paused; }
    inline bool is_running() const override { return !(_paused || _stopped); }
    inline bool is_finished() const override { return _stopped; }

    void set_config(const typename T::config_t&);

private:
    void loop();
    void update();
    void update(const typename T::config_t&);
};

// Constructors & Destructor
template <class T>
worker<T>::worker()
    : _paused(true)
    , _stopped(true)
    , _zmq_context(zmq_ctx_new())
    , _zmq_socket(op_socket_get_server(_zmq_context, ZMQ_PAIR, endpoint_prefix))
    , _task(new T)
{}

template <class T>
worker<T>::worker(worker&& w)
    : _paused(w._paused)
    , _stopped(w._stopped)
    , _zmq_context(std::move(w._zmq_context))
    , _zmq_socket(std::move(w._zmq_socket))
    , _thread(std::move(w._thread))
    , _task(std::move(w._task))
{}

template <class T>
worker<T>::~worker()
{
    std::cout << "worker::~worker()" << std::endl;
    if (_thread.joinable()) {
        _thread.detach();
        _stopped = true;
        _paused = false;
        update();
    }

    zmq_close(_zmq_socket.get());
    zmq_ctx_shutdown(_zmq_context);
    zmq_ctx_term(_zmq_context);
}

// Methods : public
template <class T>
void worker<T>::start()
{
    if (!_stopped) return;
    _stopped = false;

    std::cout << "Start worker " << _stopped << " " << _paused
              << std::endl;
    _thread = std::thread([this]() { loop(); });
    update();
}

template <class T>
void worker<T>::stop()
{
    if (_stopped) return;
    _stopped = true;
    _thread.detach();
    update();
}

template <class T>
void worker<T>::pause()
{
    if (_paused) return;
    _paused = true;
    update();
}

template <class T>
void worker<T>::resume()
{
    if (!_paused) return;
    _paused = false;
    update();
}

template <class T>
void worker<T>::set_config(const typename T::config_t& c)
{
    update(c);
}

// Methods : private
template <class T>
void worker<T>::loop()
{
    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " started" << std::endl;
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_zmq_context, ZMQ_PAIR, endpoint_prefix));

    worker::message msg{.paused = true, .stopped = false};

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
        
        if (recv == sizeof(msg)) {
            std::cout << "Thread update task config" << std::endl;
            _task->set_config(msg.config);
        }

        _task->run();
    }

    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " finished" << std::endl;
}

template <class T>
void worker<T>::update()
{
    if (is_finished()) return;

    worker::message msg{
        .paused = _paused,
        .stopped = _stopped,
        .config = _task->config()
    };

    zmq_send(_zmq_socket.get(), &msg, sizeof(msg), 0); // ZMQ_NOBLOCK);
    std::cout << "worker::update()" << std::endl;
}

template <class T>
void worker<T>::update(const typename T::config_t& c)
{
    if (is_finished()) return;

    worker::message msg{
        .paused = _paused,
        .stopped = _stopped,
        .config = c
    };

    zmq_send(_zmq_socket.get(), &msg, sizeof(msg), 0); // ZMQ_NOBLOCK);
    std::cout << "worker::update(config)" << std::endl;
}
    
} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_WORKER_HPP_
