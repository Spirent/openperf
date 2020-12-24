#ifndef _OP_FRAMEWORK_GENERATOR_WORKER_HPP_
#define _OP_FRAMEWORK_GENERATOR_WORKER_HPP_

#include <atomic>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <zmq.h>

#include "framework/core/op_log.h"
#include "framework/core/op_socket.h"
#include "framework/core/op_thread.h"
#include "framework/message/serialized_message.hpp"

#include "task.hpp"
#include "feedback_tracker.hpp"

namespace openperf::framework::generator::internal {

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
           const std::string& name);
    ~worker();

    template <typename T>
    void start(T&&, std::optional<uint16_t> core_id = std::nullopt);
    bool is_finished() const { return m_finished; }

private:
    template <typename T> void run(task<T>&);
    void send(operation_t);
    template <typename T> void send(const T&);
    template <typename T> void send(std::unique_ptr<T> stat);

    operation_t next_command(bool wait = false) noexcept;
};

//
// Implementation
//

// Methods : public
template <typename T>
void worker::start(T&& task, std::optional<uint16_t> core_id)
{
    if (!m_finished) return;

    m_finished = false;
    m_thread = std::thread([this, task = std::move(task), core_id]() mutable {
        // Set Thread name
        op_thread_setname(m_thread_name.c_str());

        // Set Thread core affinity, if specified
        if (core_id) {
            if (auto err = op_thread_set_affinity(core_id.value())) {
                OP_LOG(OP_LOG_ERROR,
                       "Cannot set worker thread affinity: %s",
                       std::strerror(err));
            }
        }

        OP_LOG(OP_LOG_DEBUG, "Worker thread started");

        run(task);
        op_log_close();
    });
}

// Methods : private
template <typename T> void worker::send(const T& stat)
{
    auto msg = message::serialized_message{};
    message::push(msg, operation_t::STATISTICS);
    message::push(msg, stat);

    if (auto r = message::send(m_statistics_socket.get(), std::move(msg)); r) {
        OP_LOG(OP_LOG_ERROR,
               "Worker ZMQ statistics socket send with error: %s",
               zmq_strerror(r));
    }
}

template <typename T> void worker::send(std::unique_ptr<T> stat)
{
    auto msg = message::serialized_message{};
    message::push(msg, operation_t::STATISTICS);
    message::push(msg, stat.release());

    if (auto r = message::send(m_statistics_socket.get(), std::move(msg)); r) {
        OP_LOG(OP_LOG_ERROR,
               "Worker ZMQ statistics socket send with error: %s",
               zmq_strerror(r));
    }
}

using namespace std::chrono_literals;

template <typename T> void worker::run(task<T>& task)
{
    using namespace std::chrono_literals;

    // Send the INIT messages to the controller periodically and
    // wait the READY message from the controller as confirmation
    while (true) {
        send(operation_t::INIT);

        // Sleep is needed to prevent storm of ZMQ messages
        std::this_thread::sleep_for(1ms);
        if (auto op = next_command(false); op == operation_t::READY) {
            // Send the READY message back to the controller
            send(operation_t::READY);
            break;
        }
    }

    for (bool paused = true; !m_finished;) {
        auto operation = next_command(paused);

        // Process the received operation
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
            break;
        case operation_t::NOOP:
            send(task.spin());
            break;
        default:
            break;
        }

        // Report the controller about processed operation
        switch (operation) {
        case operation_t::NOOP:
        case operation_t::READY:
        case operation_t::INIT:
            // NO REPORT
            break;
        default:
            send(operation);
        }
    }
}

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_WORKER_HPP_
