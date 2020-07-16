#ifndef _OP_FRAMEWORK_UTILS_GENERATOR_CONTROLLER_HPP_
#define _OP_FRAMEWORK_UTILS_GENERATOR_CONTROLLER_HPP_

#include <functional>
#include <forward_list>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <zmq.h>
#include "framework/core/op_thread.h"
#include "framework/core/op_socket.h"
#include "framework/core/op_log.h"
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

template <typename S> class controller
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
    internal::server m_server;
    // std::shared_ptr<void> m_context;
    // std::unique_ptr<void, op_socket_deleter> m_control_socket;

    std::string m_thread_name;
    std::thread m_thread;
    std::atomic_bool m_stop;
    std::function<void(const S&)> m_processor;

    std::forward_list<internal::worker> m_workers;

public:
    // Constructors & Destructor
    controller(controller&& c);
    controller(const controller&) = delete;
    explicit controller(const std::string& name = "op_ctl");
    ~controller();

    // Methods : public
    void pause() { m_server.send(internal::server::operation_t::PAUSE); }
    void resume() { m_server.send(internal::server::operation_t::RESUME); }
    void reset() { m_server.send(internal::server::operation_t::RESET); }

    template <typename T> void add(T&& t, const std::string& name = "");
    template <typename T> void processor(T&& processor);
    void clear() { m_workers.clear(); }

private:
    // Methods : private
    void loop();
    // void send_command(operation_t op);
};

// Constructors & Destructor
template <typename S>
controller<S>::controller(controller&& c)
    : m_server(std::move(c.m_server))
    //: m_context(std::move(c.m_context))
    //, m_control_socket(std::move(c.m_control_socket))
    , m_thread_name(std::move(c.m_thread_name))
    , m_thread(std::move(c.m_thread))
    , m_stop(c.m_stop.load())
    , m_processor(std::move(c.m_processor))
    , m_workers(std::move(c.m_workers))
{}

template <typename S>
controller<S>::controller(const std::string& name)
    : m_server(name)
    //: m_context(zmq_ctx_new(), zmq_ctx_deleter())
    //, m_control_socket(
    //      op_socket_get_server(m_context.get(), ZMQ_PUB, CONTROL_ENDPOINT))
    , m_thread_name(name)
{
    m_stop = false;
    m_thread = std::thread([this] {
        // Set the thread name
        op_thread_setname(m_thread_name.c_str());

        OP_LOG(OP_LOG_DEBUG, "Control thread started");

        // Run the loop of the thread
        while (!m_stop) {
            auto stats = m_server.next_statistics<S>(true);
            if (stats.has_value()) m_processor(stats.value());
        }
        // loop();
    });
}

template <typename S> controller<S>::~controller()
{
    // send_command(operation_t::STOP);
    m_server.send(internal::server::operation_t::STOP);

    m_stop = true;
    if (m_thread.joinable()) m_thread.join();
}

// Methods : public
template <typename S>
template <typename T>
void controller<S>::processor(T&& processor)
{
    m_processor = processor;
}

template <typename S>
template <typename T>
void controller<S>::add(T&& t, const std::string& name)
{
    m_workers.emplace_front(m_server.make_client(), name);
    m_workers.front().start(std::forward<T>(t));
}

// Methods : private
// template <typename S> void controller<S>::loop()
//{
//     auto socket = std::unique_ptr<void, op_socket_deleter>(
//        op_socket_get_server(m_context.get(), ZMQ_SUB, STATISTICS_ENDPOINT));
//
//     if (zmq_setsockopt(socket.get(), ZMQ_SUBSCRIBE, nullptr, 0) != 0) {
//        OP_LOG(OP_LOG_ERROR,
//               "Controller ZMQ subscribtion error: %s",
//               zmq_strerror(errno));
//        return;
//    }
//
//    while (!m_stop) {
//         auto recv = message::recv(socket.get(), ZMQ_NOBLOCK);
//
//         if (!recv && recv.error() != EAGAIN) {
//            if (errno == ETERM) {
//                OP_LOG(OP_LOG_DEBUG,
//                       "Generator controller thread %s terminated",
//                       m_thread_name.c_str());
//            } else {
//                OP_LOG(OP_LOG_ERROR,
//                       "Generator controller thread %s receive with error:
//                       %s", m_thread_name.c_str(),
//                       zmq_strerror(recv.error()));
//            }
//        }
//
//         if (recv) {
//            auto stat = message::pop<S>(recv.value());
//            m_processor(stat);
//        }
//
//    }
//};

// template <typename S> void controller<S>::send_command(operation_t op)
//{
//    auto result =
//        zmq_send(m_control_socket.get(), &op, sizeof(op), ZMQ_DONTWAIT);
//
//    if (result < 0 && errno != EAGAIN) {
//        if (errno == ETERM) {
//            OP_LOG(OP_LOG_DEBUG,
//                   "Worker thread %s terminated",
//                   m_thread_name.c_str());
//        } else {
//            OP_LOG(OP_LOG_ERROR,
//                   "Worker thread %s send with error: %s",
//                   m_thread_name.c_str(),
//                   zmq_strerror(errno));
//        }
//    }
//}

} // namespace openperf::framework::generator

#endif // _OP_FRAMEWORK_UTILS_GENERATOR_CONTROLLER_HPP_
