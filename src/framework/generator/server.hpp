#ifndef _OP_FRAMEWORK_GENERATOR_SERVER_HPP_
#define _OP_FRAMEWORK_GENERATOR_SERVER_HPP_

#include <memory>
#include <string>
#include <optional>
#include <zmq.h>
#include "framework/core/op_socket.h"
#include "framework/core/op_log.h"
#include "framework/message/serialized_message.hpp"

namespace openperf::framework::generator::internal {

class server
{
private:
    // Internal Types
    using socket_pointer = std::unique_ptr<void, op_socket_deleter>;

    struct zmq_ctx_deleter
    {
        void operator()(void* ctx) const
        {
            zmq_ctx_shutdown(ctx);
            zmq_ctx_term(ctx);
        }
    };

public:
    enum operation_t { NOOP = 0, PAUSE, RESUME, RESET, STOP };

    class client
    {
    private:
        // Attributes
        socket_pointer m_control_socket;
        socket_pointer m_statistics_socket;

    public:
        client(server::socket_pointer&& ctl, server::socket_pointer&& stat);
        client(client&&);

        operation_t next_command(bool wait = false) noexcept;
        template <typename T> void send(const T& stat);
    };

private:
    std::string m_control_endpoint;
    std::string m_statistics_endpoint;
    std::unique_ptr<void, zmq_ctx_deleter> m_context;
    socket_pointer m_control_socket;
    socket_pointer m_statistics_socket;

public:
    explicit server(const std::string& endpoint_prefix);

    client make_client() const;
    void send(operation_t operation);
    template <typename S> std::optional<S> next_statistics(bool wait = false);
};

// Implementation : template methods
template <typename T> void server::client::send(const T& stat)
{
    auto msg = message::serialized_message{};
    message::push(msg, stat);

    if (auto r = message::send(m_statistics_socket.get(), std::move(msg)); r) {
        OP_LOG(OP_LOG_ERROR, "ZMQ client send with error: %s", zmq_strerror(r));
    }
}

template <typename S> std::optional<S> server::next_statistics(bool wait)
{
    constexpr int ZMQ_BLOCK = 0;
    auto recv = message::recv(m_statistics_socket.get(),
                              (wait) ? ZMQ_BLOCK : ZMQ_NOBLOCK);

    if (!recv && recv.error() != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG, "ZMQ server socket terminated");
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "ZMQ server receive with error: %s",
                   zmq_strerror(recv.error()));
        }
    }

    return (recv) ? std::optional<S>(message::pop<S>(recv.value()))
                  : std::nullopt;
}

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_SERVER_HPP_
