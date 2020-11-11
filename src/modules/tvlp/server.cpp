#include "server.hpp"
#include "api.hpp"

#include "config/op_config_utils.hpp"

namespace openperf::tvlp::api {

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
    , m_controller_stack(std::make_unique<internal::controller_stack>(context))
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

api_reply server::handle_request(const request::tvlp::list&)
{
    auto reply = reply::tvlp::list{};
    auto list = m_controller_stack->list();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply),
                   [](const auto& config) {
                       return model::tvlp_configuration_t(config->model());
                   });

    return reply;
}

api_reply server::handle_request(const request::tvlp::get& request)
{
    auto controller = m_controller_stack->get(request.id);

    if (!controller) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    return controller.value()->model();
}

api_reply server::handle_request(const request::tvlp::erase& request)
{
    if (auto result = m_controller_stack->get(request.id); !result) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    if (auto res = m_controller_stack->erase(request.id); !res) {
        return reply::error{
            .type = reply::error::BAD_REQUEST_ERROR,
            .message = res.error(),
        };
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::tvlp::create& request)
{
    // If user did not specify an id create one for them.
    auto config = request;
    if (config.id().empty()) {
        config.id(core::to_string(core::uuid::random()));
    }

    if (auto id_check = config::op_config_validate_id_string(config.id());
        !id_check) {
        return reply::error{
            .type = reply::error::INVALID_ID,
        };
    }

    auto result = m_controller_stack->create(config);
    if (!result) {
        return reply::error{
            .type = reply::error::BAD_REQUEST_ERROR,
            .message = result.error(),
        };
    }

    return *result.value();
}

api_reply server::handle_request(const request::tvlp::start& request)
{
    auto controller = m_controller_stack->get(request.id);
    if (!controller) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    auto result = m_controller_stack->start(request.id, request.start_time);
    if (!result) {
        return reply::error{
            .type = reply::error::BAD_REQUEST_ERROR,
            .message = result.error(),
        };
    }

    return *result.value();
}

api_reply server::handle_request(const request::tvlp::stop& request)
{
    if (auto controller = m_controller_stack->get(request.id); !controller) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    auto result = m_controller_stack->stop(request.id);
    return reply::ok{};
}

api_reply server::handle_request(const request::tvlp::result::list&)
{
    auto reply = reply::tvlp::result::list{};
    auto list = m_controller_stack->results();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply),
                   [](const auto& result) { return *result; });

    return reply;
}

api_reply server::handle_request(const request::tvlp::result::get& request)
{
    auto result = m_controller_stack->result(request.id);
    if (!result) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    return *result.value();
}

api_reply server::handle_request(const request::tvlp::result::erase& request)
{
    if (auto result = m_controller_stack->result(request.id); !result) {
        return reply::error{
            .type = reply::error::NOT_FOUND,
        };
    }

    if (auto res = m_controller_stack->erase_result(request.id); !res) {
        return reply::error{
            .type = reply::error::BAD_REQUEST_ERROR,
            .message = res.error(),
        };
    }

    return reply::ok{};
}

} // namespace openperf::tvlp::api
