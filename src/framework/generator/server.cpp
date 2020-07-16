#include "server.hpp"

namespace openperf::framework::generator::internal {

server::client::client(client&& c)
    : m_control_socket(std::move(c.m_control_socket))
    , m_statistics_socket(std::move(c.m_statistics_socket))
{}

server::client::client(server::socket_pointer&& ctl,
                       server::socket_pointer&& stat)
    : m_control_socket(std::move(ctl))
    , m_statistics_socket(std::move(stat))
{}

server::operation_t server::client::next_command(bool wait) noexcept
{
    constexpr int ZMQ_BLOCK = 0;
    auto operation = operation_t::NOOP;
    auto recv = zmq_recv(m_control_socket.get(),
                         &operation,
                         sizeof(operation),
                         wait ? ZMQ_BLOCK : ZMQ_NOBLOCK);

    if (recv < 0 && errno != EAGAIN) {
        operation = operation_t::STOP;
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG, "ZMQ Socket terminated");
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "ZMQ receive with error: %s",
                   zmq_strerror(errno));
        }
    }

    return operation;
}

server::server(const std::string& endpoint_prefix)
    : m_control_endpoint("inproc://" + endpoint_prefix + "-command")
    , m_statistics_endpoint("inproc://" + endpoint_prefix + "-statistics")
    , m_context(zmq_ctx_new())
    , m_control_socket(op_socket_get_server(
          m_context.get(), ZMQ_PUB, m_control_endpoint.c_str()))
    , m_statistics_socket(op_socket_get_server(
          m_context.get(), ZMQ_SUB, m_statistics_endpoint.c_str()))

{
    if (!zmq_setsockopt(m_statistics_socket.get(), ZMQ_SUBSCRIBE, nullptr, 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ subscribtion error: %s",
               zmq_strerror(errno));
    }
}

void server::send(operation_t operation)
{
    auto result = zmq_send(
        m_control_socket.get(), &operation, sizeof(operation), ZMQ_DONTWAIT);

    if (result < 0 && errno != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG, "ZMQ terminated");
        } else {
            OP_LOG(
                OP_LOG_ERROR, "ZMQ send with error: %s", zmq_strerror(errno));
        }
    }
}

server::client server::make_client() const
{
    auto control = socket_pointer(op_socket_get_client_subscription(
        m_context.get(), m_control_endpoint.c_str(), ""));
    auto stats = socket_pointer(op_socket_get_client(
        m_context.get(), ZMQ_PUB, m_statistics_endpoint.c_str()));

    return {std::move(control), std::move(stats)};
}

} // namespace openperf::framework::generator::internal
