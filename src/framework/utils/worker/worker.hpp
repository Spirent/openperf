#ifndef _OP_UTILS_WORKER_HPP_
#define _OP_UTILS_WORKER_HPP_

#include <memory>
#include <zmq.h>
#include <thread>
#include <forward_list>
#include <atomic>
#include <cstring>
#include <optional>

#include "core/op_core.h"
#include "core/op_thread.h"
#include "utils/worker/task.hpp"

namespace openperf::utils::worker {

class workable
{
public:
    virtual ~workable() = default;

    virtual void start() = 0;
    virtual void start(int core) = 0;
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
        std::optional<typename T::config_t> config;
    };

private:
    constexpr static auto m_endpoint = "inproc://worker-p2p";

    bool m_paused = true;
    bool m_stopped = true;
    typename T::config_t m_config;
    std::unique_ptr<T> m_task;
    void* m_zmq_context;
    std::unique_ptr<void, op_socket_deleter> m_zmq_socket;
    std::thread m_thread;
    std::string m_thread_name;
    int16_t m_core;

public:
    worker(worker&&);
    worker(const worker&) = delete;
    explicit worker(const typename T::config_t&,
                    std::string_view thread_name = "uworker");
    ~worker();

    void start() override;
    void start(int core_id) override;
    void stop() override;
    void pause() override;
    void resume() override;

    bool is_paused() const override { return m_paused; }
    bool is_running() const override { return !(m_paused || m_stopped); }
    bool is_finished() const override { return m_stopped; }
    typename T::config_t config() const { return m_task->config(); }
    typename T::stat_t stat() const { return m_task->stat(); };

    void clear_stat() { m_task->clear_stat(); }
    void config(const typename T::config_t&);

private:
    void loop();
    void update();
    void send_message(const worker::message&);
};

// Constructors & Destructor
template <class T>
worker<T>::worker(const typename T::config_t& c, std::string_view name)
    : m_paused(true)
    , m_stopped(true)
    , m_config(c)
    , m_task(new T)
    , m_zmq_context(zmq_init(0))
    , m_zmq_socket(op_socket_get_server(m_zmq_context, ZMQ_PUSH, m_endpoint))
    , m_thread_name(name)
    , m_core(-1)
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
    , m_thread_name(std::move(w.m_thread_name))
    , m_core(w.m_core)
{}

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
template <class T> void worker<T>::start(int core_id)
{
    m_core = core_id;
    start();
}

template <class T> void worker<T>::start()
{
    static std::atomic_uint thread_number = 0;

    if (!m_stopped) return;
    m_stopped = false;

    m_thread = std::thread([this]() {
        // Set Thread name
        op_thread_setname(
            ("op_" + m_thread_name + "_" + std::to_string(++thread_number))
                .c_str());

        // Set Thread core affinity, if specified
        if (m_core >= 0)
            if (auto err = op_thread_set_affinity(m_core))
                OP_LOG(OP_LOG_ERROR,
                       "Cannot set worker thread affinity: %s",
                       std::strerror(err));

        loop();
    });

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
    send_message(
        worker::message{.stop = m_stopped, .pause = m_paused, .config = c});
}

// Methods : private
template <class T> void worker<T>::loop()
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(m_zmq_context, ZMQ_PULL, m_endpoint));

    for (bool paused = true;;) {
        void* msg_ptr = nullptr;
        int recv = zmq_recv(
            socket.get(), &msg_ptr, sizeof(void*), paused ? 0 : ZMQ_NOBLOCK);

        if (recv < 0 && errno != EAGAIN) {
            OP_LOG(OP_LOG_ERROR,
                   "Worker thread %s shutdown",
                   m_thread_name.c_str());
            break;
        }

        if (recv > 0) {
            // The thread takes ownership of the pointer to the message
            // and guarantees deleting message after processing.
            auto msg = std::unique_ptr<worker::message>(
                reinterpret_cast<worker::message*>(msg_ptr));

            if (msg->config) m_task->config(msg->config.value());

            if (paused && !(msg->pause || msg->stop)) m_task->resume();
            if (!paused && (msg->pause || msg->stop)) m_task->pause();

            paused = msg->pause;

            if (msg->stop) break;
            if (msg->pause) continue;
        }

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

    // A copy of the message is created on the heap and its pointer
    // is passed through ZMQ to thread. The thread will take ownership of
    // this message and should delete it after processing.
    auto pointer = new worker::message(msg);
    zmq_send(m_zmq_socket.get(), &pointer, sizeof(pointer), 0);
}

} // namespace openperf::utils::worker

#endif // _OP_UTILS_WORKER_HPP_
