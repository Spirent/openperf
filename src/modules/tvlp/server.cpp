#include "server.hpp"
#include "api.hpp"

namespace openperf::tvlp::api {

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
    , m_controller_stack(std::make_unique<internal::controller_stack>())
{
    // Setup event loop
    struct op_event_callbacks callbacks = {
        .on_read = [](const op_event_data* data, void* arg) -> int {
            auto s = reinterpret_cast<server*>(arg);
            return s->handle_rpc_request(data);
        }};
    loop.add(m_socket.get(), &callbacks, this);
}

int server::handle_rpc_request(const op_event_data* data)
{
    auto reply_errors = 0;
    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> api_reply {
            return (handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

        if (send_message(data->socket,
                         serialize(std::forward<api_reply>(reply)))
            == -1) {
            reply_errors++;
            OP_LOG(
                OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }
    }

    return ((reply_errors || errno == ETERM) ? -1 : 0);
}

api_reply server::handle_request(const request::tvlp::list&)
{
    return reply::tvlp::list{
        .data = std::make_unique<std::vector<tvlp_config_t>>()};
}

api_reply server::handle_request(const request::tvlp::get&)
{
    return reply::ok{};
}
api_reply server::handle_request(const request::tvlp::erase&)
{
    return reply::ok{};
}
api_reply server::handle_request(const request::tvlp::create& request)
{
    return reply::tvlp::item{
        .data = std::make_unique<tvlp_config_t>(*request.data)};
}
api_reply server::handle_request(const request::tvlp::start&)
{
    return reply::ok{};
}
api_reply server::handle_request(const request::tvlp::stop&)
{
    return reply::ok{};
}

} // namespace openperf::tvlp::api
