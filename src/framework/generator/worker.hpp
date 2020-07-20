#ifndef _OP_FRAMEWORK_GENERATOR_WORKER_HPP_
#define _OP_FRAMEWORK_GENERATOR_WORKER_HPP_

#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <zmq.h>

#include "framework/core/op_log.h"
#include "framework/core/op_socket.h"
#include "framework/core/op_thread.h"
#include "framework/message/serialized_message.hpp"

#include "task.hpp"

namespace openperf::framework::generator::internal {

enum class operation_t { NOOP = 0, PAUSE, RESUME, RESET, STOP };

class worker final
{
public:
    using socket_pointer = std::unique_ptr<void, op_socket_deleter>;

private:
    socket_pointer m_control_socket;
    socket_pointer m_statistics_socket;

    std::thread m_thread;
    std::string m_thread_name;
    std::atomic_bool m_finished;

public:
    worker(const worker&) = delete;
    worker(socket_pointer&& control_socket,
           socket_pointer&& statistics_socket,
           const std::string& name = "worker");
    ~worker();

    template <typename T> void start(T&&, int core_id = -1);
    bool is_finished() const { return m_finished; }

private:
    template <typename T> void run(task<T>&);
    template <typename T> void send(const T&);
    operation_t next_command(bool wait = false) noexcept;
};

//
// Implementation
//

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
template <typename T> void worker::send(const T& stat)
{
    auto msg = message::serialized_message{};
    message::push(msg, stat);

    if (auto r = message::send(m_statistics_socket.get(), std::move(msg)); r) {
        OP_LOG(OP_LOG_ERROR,
               "Worker ZMQ statistics socket send with error: %s",
               zmq_strerror(r));
    }
}

template <typename T> void worker::run(task<T>& task)
{
    for (bool paused = true; !m_finished;) {
        auto operation = next_command(paused);

        switch (operation) {
        case operation_t::STOP:
            m_finished = true;
            break;
        case operation_t::PAUSE:
            paused = true;
            break;
        case operation_t::RESET:
            task.reset();
            break;
        case operation_t::RESUME:
            paused = false;
            [[fallthrough]];
        case operation_t::NOOP:
            [[fallthrough]];
        default:
            send(task.spin());
        }
    }
}

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_WORKER_HPP_
