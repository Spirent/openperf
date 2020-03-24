#ifndef _OP_UTILS_WORKER_HPP_
#define _OP_UTILS_WORKER_HPP_

#include <zmq.h>
#include <thread>
#include <forward_list>

#include "core/op_core.h"
#include "utils/worker/task.hpp"

namespace openperf::utils::worker {

class workable
{
public:
    virtual ~workable() {}

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    virtual bool is_paused() const = 0;
    virtual bool is_running() const = 0;
    virtual bool is_finished() const = 0;
};

// Worker Template
template <class T> class worker final : public workable
{
private:
    struct message
    {
        bool stop = true;
        bool pause = true;
        bool reconf = false;
        typename T::config_t config;
    };

    static constexpr auto endpoint_prefix = "inproc://openperf-worker";

private:
    bool _paused;
    bool _stopped;
    void* _zmq_context;
    std::unique_ptr<void, op_socket_deleter> _zmq_socket;
    std::thread _thread;
    std::unique_ptr<T> _task;

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
    void send_message(const worker::message&);
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

template <class T> worker<T>::~worker()
{
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
template <class T> void worker<T>::start()
{
    if (!_stopped) return;
    _stopped = false;

    _thread = std::thread([this]() { loop(); });
    update();
}

template <class T> void worker<T>::stop()
{
    if (_stopped) return;
    _stopped = true;
    _thread.detach();
    update();
}

template <class T> void worker<T>::pause()
{
    if (_paused) return;
    _paused = true;
    update();
}

template <class T> void worker<T>::resume()
{
    if (!_paused) return;
    _paused = false;
    update();
}

template <class T> void worker<T>::set_config(const typename T::config_t& c)
{
    send_message(worker::message{
        .stop = _stopped, .pause = _paused, .reconf = true, .config = c});
}

// Methods : private
template <class T> void worker<T>::loop()
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_zmq_context, ZMQ_PAIR, endpoint_prefix));

    worker::message msg{.stop = false, .pause = true, .reconf = false};

    for (;;) {
        int recv = zmq_recv(
            socket.get(), &msg, sizeof(msg), msg.pause ? 0 : ZMQ_NOBLOCK);
        if (recv < 0 && errno != EAGAIN) {
            OP_LOG(OP_LOG_ERROR, "worker thread shutdown");
            break;
        }

        if (msg.stop) { break; }
        if (msg.reconf) { _task->set_config(msg.config); }

        _task->run();
    }
}

template <class T> void worker<T>::update()
{
    if (is_finished()) return;

    send_message(worker::message{
        .stop = _stopped,
        .pause = _paused,
    });
}

template <class T> void worker<T>::send_message(const worker::message& msg)
{
    if (is_finished()) return;

    zmq_send(_zmq_socket.get(), &msg, sizeof(msg), 0); // ZMQ_NOBLOCK);
}

} // namespace openperf::utils::worker

#endif // _OP_UTILS_WORKER_HPP_
