#include "server.hpp"
#include "api.hpp"

#include "config/op_config_utils.hpp"

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
    while (auto request = openperf::message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> api_reply {
            return (handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

        if (openperf::message::send(data->socket,
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

reply::error to_error(reply::error_data::type_t type,
                      int code = 0,
                      const std::string& value = "")
{
    return {.data = std::make_unique<reply::error_data>(
                reply::error_data{.type = type, .code = code, .value = value})};
}

api_reply server::handle_request(const request::tvlp::list&)
{
    auto reply = reply::tvlp::list{
        .data = std::make_unique<std::vector<tvlp_config_t>>()};
    auto list = m_controller_stack->list();
    std::for_each(std::begin(list), std::end(list), [&](const auto& c) {
        reply.data->push_back(tvlp_config_t(c->model()));
    });
    return reply;
}

api_reply server::handle_request(const request::tvlp::get& request)
{
    auto controller = m_controller_stack->get(request.id);

    if (!controller) { return to_error(reply::error_data::NOT_FOUND); }
    auto reply = reply::tvlp::item{
        .data = std::make_unique<tvlp_config_t>(controller.value()->model())};

    return reply;
}

api_reply server::handle_request(const request::tvlp::erase& request)
{
    auto result = m_controller_stack->get(request.id);
    if (!result) { return to_error(reply::error_data::NOT_FOUND); }

    auto res = m_controller_stack->erase(request.id);
    if (!res) {
        return to_error(reply::error_data::BAD_REQUEST_ERROR, 0, res.error());
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::tvlp::create& request)
{
    // If user did not specify an id create one for them.
    if (request.data->id().empty()) {
        request.data->id(core::to_string(core::uuid::random()));
    }

    if (auto id_check =
            config::op_config_validate_id_string(request.data->id());
        !id_check)
        return to_error(reply::error_data::INVALID_ID);

    auto result = m_controller_stack->create(*request.data);
    if (!result) {
        return (
            to_error(reply::error_data::BAD_REQUEST_ERROR, 0, result.error()));
    }
    auto reply = reply::tvlp::item{
        .data = std::make_unique<tvlp_config_t>(*result.value())};
    return reply;
}

api_reply server::handle_request(const request::tvlp::start& request)
{
    auto controller = m_controller_stack->get(request.data->id);
    if (!controller) { return to_error(reply::error_data::NOT_FOUND); }

    auto result =
        m_controller_stack->start(request.data->id, request.data->start_time);
    if (!result) {
        return to_error(
            reply::error_data::BAD_REQUEST_ERROR, 0, result.error());
    }
    auto reply = reply::tvlp::result::item{
        .data = std::make_unique<tvlp_result_t>(*result.value())};

    return reply;
}

api_reply server::handle_request(const request::tvlp::stop& request)
{
    auto controller = m_controller_stack->get(request.id);
    if (!controller) { return to_error(reply::error_data::NOT_FOUND); }

    auto result = m_controller_stack->stop(request.id);
    return reply::ok{};
}

api_reply server::handle_request(const request::tvlp::result::list&)
{
    auto reply = reply::tvlp::result::list{
        .data = std::make_unique<std::vector<tvlp_result_t>>()};
    auto list = m_controller_stack->results();
    std::for_each(std::begin(list), std::end(list), [&](const auto& c) {
        reply.data->push_back(*c);
    });
    return reply;
}

api_reply server::handle_request(const request::tvlp::result::get& request)
{
    auto result = m_controller_stack->result(request.id);

    if (!result) { return to_error(reply::error_data::NOT_FOUND); }
    auto reply = reply::tvlp::result::item{
        .data = std::make_unique<tvlp_result_t>(*result.value())};

    return reply;
}

api_reply server::handle_request(const request::tvlp::result::erase& request)
{
    auto result = m_controller_stack->result(request.id);
    if (!result) { return to_error(reply::error_data::NOT_FOUND); }

    auto res = m_controller_stack->erase_result(request.id);
    if (!res) {
        return to_error(reply::error_data::BAD_REQUEST_ERROR, 0, res.error());
    }

    return reply::ok{};
}

} // namespace openperf::tvlp::api
