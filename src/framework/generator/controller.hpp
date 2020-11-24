#ifndef _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_
#define _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_

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

namespace openperf::framework::generator {

/**
 * Messages:
 *   CONTROLLER -> WORKER
 *     STOP
 *     PAUSE
 *     RESUME
 *     RESET
 *
 *   WORKER -> CONTROLLER
 *     STATISTICS
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
    std::unique_ptr<void, op_socket_deleter> m_control_socket;
    std::unique_ptr<void, op_socket_deleter> m_statistics_socket;

    std::string m_thread_name;
    std::thread m_thread;
    std::atomic_bool m_stop;

    std::forward_list<internal::worker> m_workers;

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

    template <typename S, typename T>
    std::enable_if_t<!std::is_pointer_v<S>, void> start(T&& processor);
    template <typename S, typename T>
    std::enable_if_t<std::is_pointer_v<S>, void> start(T&& processor);

private:
    // Methods : private
    void send(internal::operation_t);

    template <typename S>
    std::enable_if_t<!std::is_pointer_v<S>, std::optional<S>>
    next_statistics(bool wait = false);
    template <typename S>
    std::enable_if_t<std::is_pointer_v<S>, S>
    next_statistics(bool wait = false);
};

//
// Implementation
//

// Methods : public
template <typename S, typename T>
std::enable_if_t<!std::is_pointer_v<S>, void> controller::start(T&& processor)
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
                if (auto stats = next_statistics<S>(true); stats)
                    processor(stats.value());
            } catch (const std::runtime_error& e) {
                if (!m_stop) throw e;
            }
        }

        op_log_close();
    });
}

template <typename S, typename T>
std::enable_if_t<std::is_pointer_v<S>, void> controller::start(T&& processor)
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
                if (auto stats = next_statistics<S>(true); stats != nullptr) {
                    processor(*stats);
                    delete stats;
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
    auto control =
        internal::worker::socket_pointer(op_socket_get_client_subscription(
            m_context.get(), m_control_endpoint.c_str(), ""));
    auto stats = internal::worker::socket_pointer(op_socket_get_client(
        m_context.get(), ZMQ_PUB, m_statistics_endpoint.c_str()));

    auto& worker =
        m_workers.emplace_front(std::move(control), std::move(stats), name);
    worker.start(std::forward<T>(task), core);
}

// Methods : private
template <typename S>
std::enable_if_t<!std::is_pointer_v<S>, std::optional<S>>
controller::next_statistics(bool wait)
{
    constexpr int ZMQ_WAIT = 0;
    auto recv = message::recv(m_statistics_socket.get(),
                              wait ? ZMQ_WAIT : ZMQ_DONTWAIT);

    if (!recv && recv.error() != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG,
                   "Controller ZMQ statistics socket %s terminated",
                   m_statistics_endpoint.c_str());
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Controller ZMQ statistics socket %s receive with error: %s",
                   m_statistics_endpoint.c_str(),
                   zmq_strerror(recv.error()));
        }

        throw std::runtime_error(zmq_strerror(recv.error()));
    }

    return recv ? std::optional<S>(message::pop<S>(recv.value()))
                : std::nullopt;
}

template <typename S>
std::enable_if_t<std::is_pointer_v<S>, S> controller::next_statistics(bool wait)
{
    constexpr int ZMQ_WAIT = 0;
    auto recv = message::recv(m_statistics_socket.get(),
                              wait ? ZMQ_WAIT : ZMQ_DONTWAIT);

    if (!recv && recv.error() != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG,
                   "Controller ZMQ statistics socket %s terminated",
                   m_statistics_endpoint.c_str());
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Controller ZMQ statistics socket %s receive with error: %s",
                   m_statistics_endpoint.c_str(),
                   zmq_strerror(recv.error()));
        }

        throw std::runtime_error(zmq_strerror(recv.error()));
    }

    return recv ? message::pop<S>(recv.value()) : nullptr;
}

} // namespace openperf::framework::generator

#endif // _OP_FRAMEWORK_GENERATOR_CONTROLLER_HPP_
