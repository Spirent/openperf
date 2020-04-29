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
        return (to_error(error_type::NOT_FOUND));

    auto result = m_generator_stack->create(*request.source);
    if (!result) { return to_error(error_type::NOT_FOUND); }

    auto reply = reply_cpu_generators{};
    auto reply_generator_model = model::generator(*result.value());
    reply.generators.emplace_back(std::make_unique<model::generator>(reply_generator_model));
    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_del& request)
{
    m_generator_stack->erase(request.id);
    return reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_start& request)
{
    auto generator = m_generator_stack->generator(request.id);

    if (!generator) { return to_error(api::error_type::NOT_FOUND); }
    generator->start();

    //TODO:
    auto reply_model = model::generator_result();
    //    m_generator_stack->statistics(request.id));

    auto reply = reply_cpu_generator_results{};
    reply.results.emplace_back(std::make_unique<model::generator_result>(reply_model));
    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_stop& request)
{
    auto generator = m_generator_stack->generator(request.id);

    if (!generator) { return api::reply_ok{}; }
    generator->stop();

    return api::reply_ok{};
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_start& request)
{
    auto reply = reply_cpu_generator_results{};

    for (size_t i = 0; i < request.ids->size(); ++i)
    {
        auto generator = m_generator_stack->generator(request.ids->at(i));
        if (!generator) {
            for (size_t j = 0; j < i; ++j) {
                auto generator_to_stop = m_generator_stack->generator(request.ids->at(i));
                if (generator_to_stop) {
                    generator_to_stop->stop();
                }
            }
            return to_error(api::error_type::NOT_FOUND, 0,
                "Generator " + request.ids->at(i) + " not found");
        }
        generator->start();

        //TODO:
        auto reply_model = model::generator_result();
        //    m_generator_stack->statistics(request.ids->at(i)));

        reply.results.emplace_back(std::make_unique<model::generator_result>(reply_model));
    }

    return reply;
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_stop& request)
{
   for (auto & id : *request.ids)
    {
        auto generator = m_generator_stack->generator(id);
        if (!generator) {
            continue;
        }
        generator->stop();
    }

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_result_list&)
{
    auto reply = reply_cpu_generator_results{};
    for (auto generator : m_generator_stack->list()) {
        auto reply_model = model::generator_result(generator->statistics());
        reply.results.emplace_back(std::make_unique<model::generator_result>(reply_model));
    }

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_result& request)
{
    auto generator = m_generator_stack->generator(request.id);

    if (!generator) { return to_error(api::error_type::NOT_FOUND); }

    auto reply = reply_cpu_generator_results{};
    auto reply_model = model::generator_result(generator->statistics());
    reply.results.emplace_back(std::make_unique<model::generator_result>(reply_model));

    return reply;
}

reply_msg
server::handle_request(const request_cpu_generator_result_del& request)
{
    m_generator_stack->erase_statistics(request.id);
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
