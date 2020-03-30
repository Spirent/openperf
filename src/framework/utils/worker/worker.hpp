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

    bool m_paused;
    bool m_stopped;
    typename T::config_t m_config;
    std::unique_ptr<T> m_task;
    void* m_zmq_context;
    std::unique_ptr<void, op_socket_deleter> m_zmq_socket;
    std::thread m_thread;

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

    inline bool is_paused() const override { return m_paused; }
    inline bool is_running() const override { return !(m_paused || m_stopped); }
    inline bool is_finished() const override { return m_stopped; }
    inline typename T::config_t config() const { return m_task->config(); }
    inline typename T::stat_t stat() const { return m_task->stat(); };
    inline void clear_stat() { return m_task->clear_stat(); };

    inline void clear_stat() { _task->clear_stat(); }
    void config(const typename T::config_t&);

private:
    void loop();
    void update();
    void send_message(const worker::message&);
};

// Constructors & Destructor
template <class T>
worker<T>::worker()
    : m_paused(true)
    , m_stopped(true)
    , m_task(new T)
    , m_zmq_context(zmq_init(0))
    , m_zmq_socket(op_socket_get_server(m_zmq_context, ZMQ_PAIR, _endpoint))
{}

template <class T>
worker<T>::worker(worker&& w)
    : m_paused(w.m_paused)
    , m_stopped(w.m_stopped)
    , m_config(std::move(w.m_config))
    , m_task(std::move(w.m_task))
    , m_zmq_context(std::move(w.m_zmq_context))
    , m_zmq_socket(std::move(w.m_zmq_socket))
    , m_thread(std::move(w.m_thread))
{}

template <class T>
worker<T>::worker(const typename T::config_t& c)
    : worker()
{
    m_config = c;
}

template <class T> worker<T>::~worker()
{
    if (m_thread.joinable()) {
        m_stopped = true;
        m_paused = false;
        update();
        m_thread.detach();
    }

    zmq_close(m_zmq_socket.get());
    zmq_ctx_shutdown(m_zmq_context);
    zmq_ctx_term(m_zmq_context);
}

// Methods : public
template <class T> void worker<T>::start()
{
    if (!m_stopped) return;
    m_stopped = false;

    m_thread = std::thread([this]() { loop(); });
    config(m_config);
}

template <class T> void worker<T>::stop()
{
    if (m_stopped) return;
    send_message(worker::message{.stop = true, .pause = m_paused});
    m_stopped = true;
    m_thread.join();
}

template <class T> void worker<T>::pause()
{
    if (m_paused) return;
    m_paused = true;
    update();
}

template <class T> void worker<T>::resume()
{
    if (!m_paused) return;
    m_paused = false;
    update();
}

template <class T> void worker<T>::config(const typename T::config_t& c)
{
    m_config = c;
    if (is_finished()) return;
    send_message(worker::message{
        .stop = m_stopped, .pause = m_paused, .reconf = true, .config = c});
}

// Methods : private
template <class T> void worker<T>::loop()
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(m_zmq_context, ZMQ_PAIR, _endpoint));

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
            m_task->config(msg.config);
            msg.reconf = false;
        }

        if (paused && !(msg.pause || msg.stop)) { m_task->resume(); }

        if (!paused && (msg.pause || msg.stop)) { m_task->pause(); }

        paused = msg.pause;

        if (msg.stop) { break; }
        if (msg.pause) { continue; }

        m_task->spin();
    }
}

template <class T> void worker<T>::update()
{
    if (is_finished()) return;

    send_message(worker::message{
        .stop = m_stopped,
        .pause = m_paused,
    });
}

template <class T> void worker<T>::send_message(const worker::message& msg)
{
    if (is_finished()) return;

    zmq_send(m_zmq_socket.get(), &msg, sizeof(msg), 0); // ZMQ_NOBLOCK);
}

} // namespace openperf::utils::worker

#endif // _OP_UTILS_WORKER_HPP_
