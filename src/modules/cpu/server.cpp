#include <zmq.h>

#include "api.hpp"
#include "server.hpp"
#include "config/op_config_utils.hpp"
#include "cpu_info.hpp"

namespace openperf::cpu::api {

reply_msg server::handle_request(const request_cpu_generator_list&)
{
    auto reply = reply_cpu_generators{};
    for (auto generator : m_generator_stack->list()) {
        auto reply_generator_model = model::generator(*generator);
        reply.generators.emplace_back(std::make_unique<model::generator>(reply_generator_model));
    }

    return reply;
}


reply_msg server::handle_request(const request_cpu_generator& request)
{
    auto generator = m_generator_stack->generator(request.id);

    if (!generator) { return to_error(api::error_type::NOT_FOUND); }

    auto reply = reply_cpu_generators{};
    auto reply_generator_model = model::generator(*generator);
    reply.generators.emplace_back(std::make_unique<model::generator>(reply_generator_model));

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_add& request)
{
    // If user did not specify an id create one for them.
    if (request.source->id().empty()) {
        request.source->id(core::to_string(core::uuid::random()));
    }

    if (auto id_check = config::op_config_validate_id_string(request.source->id());
        !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0,  "Id is not valid"));

    auto result = m_generator_stack->create(*request.source);
    if (!result) { return to_error(error_type::CUSTOM_ERROR, 0, result.error()); }

    auto reply = reply_cpu_generators{};
    auto reply_generator_model = model::generator(*result.value());
    reply.generators.emplace_back(std::make_unique<model::generator>(reply_generator_model));
    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_del& request)
{
    if (auto id_check = config::op_config_validate_id_string(request.id); !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));

    if (!m_generator_stack->erase(request.id)) {
        return to_error(api::error_type::NOT_FOUND);
    }

    return reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_start& request)
{
    auto generator = m_generator_stack->generator(request.id);
    if (!generator) { return to_error(api::error_type::NOT_FOUND); }

    auto result = m_generator_stack->start_generator(request.id);
    if (!result) { return to_error(api::error_type::CUSTOM_ERROR, 0, result.error()); }

    auto reply = reply_cpu_generator_results{};
    reply.results.emplace_back(std::make_unique<model::generator_result>(result.value()));
    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_stop& request)
{
    if (auto id_check = config::op_config_validate_id_string(request.id); !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0,  "Id is not valid"));

    if (!m_generator_stack->stop_generator(request.id)) {
        return to_error(api::error_type::NOT_FOUND);
    }

    return reply_ok{};
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_start& request)
{
    auto reply = reply_cpu_generator_results{};

    for (auto& id : *request.ids)
    {
        auto stats = m_generator_stack->start_generator(id);
        if (!stats) {
            for (auto& stat : reply.results) {
                m_generator_stack->stop_generator(stat->generator_id());
                m_generator_stack->erase_statistics(stat->id());
            }
            return to_error(api::error_type::CUSTOM_ERROR, 0, stats.error());
        }

        auto reply_model = model::generator_result(stats.value());
        reply.results.emplace_back(std::make_unique<model::generator_result>(reply_model));
    }

     return reply;
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_stop& request)
{
   bool failed = false;
    for (auto& id : *request.ids) {
        failed = failed || !m_generator_stack->stop_generator(id);
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR, 0, "Some generators from the list were not found");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_result_list&)
{
    auto reply = reply_cpu_generator_results{};
    for (auto stat : m_generator_stack->list_statistics()) {
        reply.results.emplace_back(std::make_unique<model::generator_result>(stat));
    }

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_result& request)
{
    auto reply = reply_cpu_generator_results{};
    auto stat = m_generator_stack->statistics(request.id);

    if (!stat) { return to_error(api::error_type::NOT_FOUND); }
    reply.results.emplace_back(std::make_unique<model::generator_result>(stat.value()));

    return reply;
}

reply_msg
server::handle_request(const request_cpu_generator_result_del& request)
{
    if (auto id_check = config::op_config_validate_id_string(request.id); !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));
    if (!m_generator_stack->erase_statistics(request.id)) {
        return to_error(api::error_type::NOT_FOUND);
    }
    return reply_ok{};
}

reply_msg server::handle_request(const request_cpu_info&)
{
    auto reply = reply_cpu_info{
        .info = std::make_unique<model::cpu_info>()
    };
    reply.info->architecture(info::architecture());
    reply.info->cores(info::cores_count());
    reply.info->cache_line_size(info::cache_line_size());

    return reply;
}

static int _handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);
        if (send_message(data->socket, serialize_reply(std::move(reply))) == -1) {
            reply_errors++;
            OP_LOG(
                OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }
    }

    return ((reply_errors || errno == ETERM) ? -1 : 0);
}

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_generator_stack(std::make_unique<generator_stack>())
{

    struct op_event_callbacks callbacks = {.on_read = _handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

} // namespace openperf::cpu::api
