#include <zmq.h>

#include "server.hpp"
#include "cpu.hpp"
#include "framework/config/op_config_utils.hpp"
#include "framework/message/serialized_message.hpp"

namespace openperf::cpu::api {

using namespace openperf::cpu::internal;

reply_msg server::handle_request(const request_cpu_generator_list&)
{
    auto reply = reply_cpu_generators{};

    auto list = m_generator_stack.list();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply.generators),
                   [](const auto& i) {
                       return std::make_unique<model::generator>(
                           model::generator(*i));
                   });

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator& request)
{
    if (auto gen = m_generator_stack.generator(request.id); gen) {
        auto reply = reply_cpu_generators{};
        reply.generators.emplace_back(std::make_unique<model::generator>(*gen));

        return reply;
    }

    return to_error(api::error_type::NOT_FOUND);
}

reply_msg server::handle_request(const request_cpu_generator_add& request)
{
    // If user did not specify an id create one for them.
    auto& src = request.source;
    if (src->id().empty())
        src->id(core::to_string(core::uuid::random()));
    else if (!config::op_config_validate_id_string(src->id()))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    auto result = m_generator_stack.create(*src);
    if (!result) return to_error(error_type::CUSTOM_ERROR, 0, result.error());

    auto reply = reply_cpu_generators{};
    reply.generators.emplace_back(
        std::make_unique<model::generator>(*result.value()));

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_del& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.erase(request.id))
        return to_error(api::error_type::NOT_FOUND);

    return reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_bulk_add& request)
{
    auto reply = reply_cpu_generators{};

    auto remove_created_items = [&]() -> auto
    {
        for (const auto& item : reply.generators) {
            m_generator_stack.erase(item->id());
        }
    };

    for (const auto& source : request.generators) {
        // If user did not specify an id create one for them.
        if (source->id().empty()) {
            source->id(core::to_string(core::uuid::random()));
        }

        if (auto id_check = config::op_config_validate_id_string(source->id());
            !id_check) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));
        }

        auto result = m_generator_stack.create(*source);
        if (!result) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, result.error()));
        }
        reply.generators.emplace_back(
            std::make_unique<model::generator>(*result.value()));
    }

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_bulk_del& request)
{
    bool failed = false;
    for (const auto& id : request.ids) {
        failed = !m_generator_stack.erase(*id) || failed;
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some generators from the list cannot be deleted");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_start& request)
{
    if (!m_generator_stack.generator(request.id))
        return to_error(api::error_type::NOT_FOUND);

    auto result = m_generator_stack.start_generator(request.id);
    if (!result)
        return to_error(api::error_type::CUSTOM_ERROR, 0, result.error());

    auto reply = reply_cpu_generator_results{};
    reply.results.emplace_back(
        std::make_unique<model::generator_result>(result.value()));

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_stop& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.stop_generator(request.id))
        return to_error(api::error_type::NOT_FOUND);

    return reply_ok{};
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_start& request)
{
    for (const auto& id : request.ids)
        if (!m_generator_stack.generator(*id))
            return to_error(api::error_type::NOT_FOUND,
                            0,
                            "Generator from the list with ID '" + *id
                                + "' was not found");

    using string_pair = std::pair<std::string, std::string>;
    std::forward_list<string_pair> not_runned_before;
    auto rollback = [&not_runned_before, this]() {
        for (const auto& pair : not_runned_before) {
            m_generator_stack.stop_generator(pair.first);
            m_generator_stack.erase_statistics(pair.second);
        }
    };

    auto reply = reply_cpu_generator_results{};
    for (const auto& id : request.ids) {
        auto gen = m_generator_stack.generator(*id);
        if (gen->running()) continue;

        auto stats = m_generator_stack.start_generator(*id);
        if (!stats) {
            rollback();
            return to_error(api::error_type::CUSTOM_ERROR, 0, stats.error());
        }

        not_runned_before.push_front(std::make_pair(*id, stats->id()));

        reply.results.emplace_back(
            std::make_unique<model::generator_result>(stats.value()));
    }

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_bulk_stop& request)
{
    for (const auto& id : request.ids)
        if (!m_generator_stack.generator(*id))
            return to_error(api::error_type::NOT_FOUND,
                            0,
                            "Generator from the list with ID '" + *id
                                + "' was not found");

    for (const auto& id : request.ids) m_generator_stack.stop_generator(*id);

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_cpu_generator_result_list&)
{
    auto reply = reply_cpu_generator_results{};

    auto list = m_generator_stack.list_statistics();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply.results),
                   [](const auto& i) {
                       return std::make_unique<model::generator_result>(i);
                   });

    return reply;
}

reply_msg server::handle_request(const request_cpu_generator_result& request)
{
    if (auto stat = m_generator_stack.statistics(request.id); stat) {
        auto reply = reply_cpu_generator_results{};
        reply.results.emplace_back(
            std::make_unique<model::generator_result>(stat.value()));

        return reply;
    }

    return to_error(api::error_type::NOT_FOUND);
}

reply_msg
server::handle_request(const request_cpu_generator_result_del& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.statistics(request.id))
        return to_error(api::error_type::NOT_FOUND);

    if (!m_generator_stack.erase_statistics(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "Statistics are active");

    return reply_ok{};
}

reply_msg server::handle_request(const request_cpu_info&)
{
    auto info = std::make_unique<cpu_info_t>();
    info->architecture = cpu_architecture();
    info->cores = cpu_cores();
    info->cache_line_size = cpu_cache_line_size();

    return reply_cpu_info{.info = std::move(info)};
}

static int _handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> reply_msg {
            return s->handle_request(request);
        };
        auto reply = std::visit(request_visitor, *request);
        if (message::send(data->socket, serialize_reply(std::move(reply)))
            == -1) {
            ++reply_errors;
            OP_LOG(
                OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }
    }

    return (reply_errors || errno == ETERM) ? -1 : 0;
}

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
{
    struct op_event_callbacks callbacks = {.on_read = _handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

} // namespace openperf::cpu::api
