#ifndef _OP_UTILS_WORKER_HPP_
#define _OP_UTILS_WORKER_HPP_

#include <atomic>
#include <cstring>
#include <string>
#include <thread>
#include <zmq.h>

#include "framework/core/op_core.h"
#include "framework/core/op_thread.h"
#include "framework/message/serialized_message.hpp"
#include "task.hpp"
#include "server.hpp"

namespace openperf::framework::generator::internal {

// Constants
// constexpr auto CONTROL_ENDPOINT = "inproc://worker-control";
// constexpr auto STATISTICS_ENDPOINT = "inproc://worker-stats";
//
// enum class operation_t { NOOP = 0, PAUSE, RESUME, RESET, STOP };

// Worker
class worker final
{
private:
    std::atomic_bool m_paused;
    std::atomic_bool m_finished;

    internal::server::client m_client;
    // std::weak_ptr<void> m_context;
    // std::unique_ptr<void, op_socket_deleter> m_stats_socket;

    std::thread m_thread;
    std::string m_thread_name;

public:
    worker(worker&&);
    worker(const worker&) = delete;
    explicit worker(internal::server::client&&,
                    const std::string& name = "worker");

    template <typename T> void start(T&&, int core_id = -1);

    void stop();
    void pause();
    void resume();

    bool is_paused() const { return m_paused; }
    bool is_finished() const { return m_finished; }

private:
    template <typename T> void run(task<T>&);
    template <typename T> void send(const T&);
};

// Methods : public
template <typename T> void worker::start(T&& task, int core_id)
{
    if (!m_finished) return;

    m_finished = false;
    m_thread = std::thread([this, task = std::move(task), core_id]() mutable {
        // Set Thread name
        op_thread_setname(m_thread_name.c_str());

        // Set Thread core affinity, if specified
        if (core_id >= 0) {
            if (auto err = op_thread_set_affinity(core_id))
                OP_LOG(OP_LOG_ERROR,
                       "Cannot set worker thread affinity: %s",
                       std::strerror(err));
        }

        OP_LOG(OP_LOG_DEBUG, "Worker thread started");

        run(task);
    });
}

// Methods : private
template <typename T> void worker::run(task<T>& task)
{
    // constexpr int ZMQ_BLOCK = 0;
    // auto socket = std::unique_ptr<void, op_socket_deleter>(
    //    op_socket_get_client_subscription(
    //        m_context.lock().get(), CONTROL_ENDPOINT, ""));

    for (m_paused = true; !m_finished;) {
        auto operation = m_client.next_command(m_paused);
        // auto recv = zmq_recv(socket.get(),
        //                     &operation,
        //                     sizeof(operation),
        //                     m_paused ? ZMQ_BLOCK : ZMQ_NOBLOCK);

        // if (recv < 0 && errno != EAGAIN) {
        //    operation = operation_t::STOP;
        //    if (errno == ETERM) {
        //        OP_LOG(OP_LOG_DEBUG,
        //               "Worker thread %s terminated",
        //               m_thread_name.c_str());
        //    } else {
        //        OP_LOG(OP_LOG_ERROR,
        //               "Worker thread %s receive with error: %s",
        //               m_thread_name.c_str(),
        //               zmq_strerror(errno));
        //    }
        //}

        switch (operation) {
        case server::operation_t::STOP:
            task.pause();
            m_finished = true;
            break;
        case server::operation_t::PAUSE:
            if (!m_paused) task.pause();
            m_paused = true;
            break;
        case server::operation_t::RESET:
            task.reset();
            [[fallthrough]];
        case server::operation_t::RESUME:
            if (m_paused) task.resume();
            m_paused = false;
            [[fallthrough]];
        case server::operation_t::NOOP:
            [[fallthrough]];
        default:
            m_client.send(task.spin());
            // send(task.spin());
        }
    }
}

// template <typename T> void worker::send(const T& stat)
//{
//    auto msg = message::serialized_message{};
//    message::push(msg, stat);
//
//    if (auto code = message::send(m_stats_socket.get(), std::move(msg)); code)
//    {
//        OP_LOG(OP_LOG_ERROR,
//               "Worker thread %s send with error: %s",
//               m_thread_name.c_str(),
//               zmq_strerror(code));
//    }
//}

} // namespace openperf::framework::generator::internal

#endif // _OP_UTILS_WORKER_HPP_
