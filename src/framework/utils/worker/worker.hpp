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

private:
    constexpr static auto _endpoint = "inproc://worker-p2p";

    bool _paused;
    bool _stopped;
    typename T::config_t _config;
    std::unique_ptr<T> _task;
    void* _zmq_context;
    std::unique_ptr<void, op_socket_deleter> _zmq_socket;
    std::thread _thread;

public:
    worker();
    worker(worker&&);
    worker(const worker&) = delete;
    explicit worker(const typename T::config_t&);
    ~worker();

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;

    inline bool is_paused() const override { return _paused; }
    inline bool is_running() const override { return !(_paused || _stopped); }
    inline bool is_finished() const override { return _stopped; }
    inline typename T::config_t config() const { return _task->config(); }
    inline typename T::stat_t stat() const { return _task->stat(); };
    inline void clear_stat() { return _task->clear_stat(); };

    void config(const typename T::config_t&);

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
    , _task(new T)
    , _zmq_context(zmq_ctx_new())
    , _zmq_socket(op_socket_get_server(_zmq_context, ZMQ_PAIR, _endpoint))
{}

template <class T>
worker<T>::worker(worker&& w)
    : _paused(w._paused)
    , _stopped(w._stopped)
    , _config(std::move(w._config))
    , _task(std::move(w._task))
    , _zmq_context(std::move(w._zmq_context))
    , _zmq_socket(std::move(w._zmq_socket))
    , _thread(std::move(w._thread))
{}

template <class T>
worker<T>::worker(const typename T::config_t& c)
    : worker()
{
    _config = c;
}

template <class T> worker<T>::~worker()
{
    if (_thread.joinable()) {
        _stopped = true;
        _paused = false;
        update();
        //_thread.join();
        _thread.detach();
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
    config(_config);
}

template <class T> void worker<T>::stop()
{
    if (_stopped) return;
    send_message(worker::message{.stop = true, .pause = _paused});
    _stopped = true;
    _thread.join();
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

template <class T> void worker<T>::config(const typename T::config_t& c)
{
    _config = c;
    if (is_finished()) return;
    send_message(worker::message{
        .stop = _stopped, .pause = _paused, .reconf = true, .config = c});
}

// Methods : private
template <class T> void worker<T>::loop()
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_zmq_context, ZMQ_PAIR, _endpoint));

    worker::message msg{.stop = false, .pause = true, .reconf = false};
    bool paused = true;

    for (;;) {
        int recv = zmq_recv(
            socket.get(), &msg, sizeof(msg), paused ? 0 : ZMQ_NOBLOCK);
        if (recv < 0 && errno != EAGAIN) {
            OP_LOG(OP_LOG_ERROR, "worker thread shutdown");
            break;
        }

        if (msg.reconf) {
            _task->config(msg.config);
            msg.reconf = false;
        }

        if (paused && !(msg.pause || msg.stop)) { _task->resume(); }

        if (!paused && (msg.pause || msg.stop)) { _task->pause(); }

        paused = msg.pause;

        if (msg.stop) { break; }
        if (msg.pause) { continue; }

        _task->spin();
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
