#ifndef _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_
#define _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_

#include <cassert>
#include <forward_list>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>

#include <zmq.h>

#include "framework/core/op_log.h"
#include "framework/core/op_socket.h"
#include "framework/core/op_thread.h"
#include "framework/message/serialized_message.hpp"

#include "worker.hpp"
#include "feedback_tracker.hpp"

namespace openperf::framework::generator {

/**
 * Messages:
 *   CONTROLLER -> WORKER
 *     STOP
 *     PAUSE
 *     RESUME
 *     RESET
 *     READY
 *
 *   WORKER -> CONTROLLER
 *     INIT
 *     STATISTICS
 *     as confirmation:
 *       STOP
 *       PAUSE
 *       RESUME
 *       RESET
 *       READY
 */

// template <typename S> class controller
class controller final
{
private:
    // Internal Types
    struct zmq_ctx_deleter
    {
        void operator()(void* ctx) const
        {
            zmq_ctx_shutdown(ctx);
            zmq_ctx_term(ctx);
        }
    };

private:
    // Attributes
    std::unique_ptr<void, zmq_ctx_deleter> m_context;
    std::string m_control_endpoint;
    std::string m_statistics_endpoint;
    internal::worker::socket_pointer m_control_socket;
    internal::worker::socket_pointer m_statistics_socket;

    std::string m_thread_name;
    std::thread m_thread;
    std::atomic_bool m_stop;
    internal::feedback_tracker m_feedback;
    internal::operation_t m_feedback_operation;

    std::forward_list<internal::worker> m_workers;
    size_t m_worker_count = 0;

public:
    // Constructors & Destructor
    controller(const controller&) = delete;
    explicit controller(const std::string& name);
    ~controller();

    // Methods : public
    void clear();
    void pause() { send(internal::operation_t::PAUSE); }
    void resume() { send(internal::operation_t::RESUME); }
    void reset() { send(internal::operation_t::RESET); }

    template <typename T>
    void add(T&& task,
             const std::string& name,
             std::optional<uint16_t> core = std::nullopt);

    template <typename S, typename T> void start(T&& processor);

private:
    // Methods : private
    std::optional<message::serialized_message> receive(bool wait = true);
    void send(internal::operation_t, bool wait = true);
};

//
// Implementation
//

// Methods : public
template <typename S, typename T> void controller::start(T&& processor)
{
    m_stop = false;
    m_thread = std::thread([this, processor = std::move(processor)] {
        // Set the thread name
        op_thread_setname(m_thread_name.c_str());

        OP_LOG(
            OP_LOG_DEBUG, "Control thread %s started", m_thread_name.c_str());

        // Run the loop of the thread
        while (!m_stop) {
            try {
                auto msg = receive(true);
                if (!msg) continue;

                auto operation =
                    message::pop<internal::operation_t>(msg.value());

                switch (operation) {
                case internal::operation_t::STATISTICS: {
                    if constexpr (std::is_pointer_v<S>) {
                        auto stats = message::pop<S>(msg.value());
                        processor(*stats);
                        delete stats;
                    } else {
                        auto stats = message::pop<S>(msg.value());
                        processor(stats);
                    }
                } break;
                case internal::operation_t::INIT:
                    send(internal::operation_t::READY, false);
                    break;
                default:
                    if (m_feedback_operation == operation)
                        m_feedback.count_down();
                    break;
                }
            } catch (const std::runtime_error& e) {
                if (!m_stop) throw e;
            }
        }

        op_log_close();
    });
}

template <typename T>
void controller::add(T&& task,
                     const std::string& name,
                     std::optional<uint16_t> core)
{
    assert(!m_stop);

    auto control =
        internal::worker::socket_pointer(op_socket_get_client_subscription(
            m_context.get(), m_control_endpoint.c_str(), ""));
    auto stats = internal::worker::socket_pointer(op_socket_get_client(
        m_context.get(), ZMQ_PUB, m_statistics_endpoint.c_str()));

    auto& worker =
        m_workers.emplace_front(std::move(control), std::move(stats), name);
    worker.start(std::forward<T>(task), core);

    // Wait the one READY message as confirmation establishing connection
    m_worker_count++;
    m_feedback_operation = internal::operation_t::READY;
    m_feedback.init(1);
    m_feedback.wait();
}

} // namespace openperf::framework::generator

#endif // _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_
