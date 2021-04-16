#include <forward_list>

#include <zmq.h>

#include "config/op_config_utils.hpp"
#include "core/op_event_loop.hpp"
#include "network/generator.hpp"
#include "network/models/generator.hpp"
#include "network/models/server.hpp"
#include "network/internal_server.hpp"
#include "network/server.hpp"
#include "message/serialized_message.hpp"

namespace openperf::network::api {

// using namespace openperf::network::internal;

reply_msg server::handle_request(const request::generator::list&)
{
    auto reply = reply::generator::list{};

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

reply_msg server::handle_request(const request::generator::get& request)
{
    if (auto gen = m_generator_stack.generator(request.id); gen) {
        auto reply = reply::generator::list{};
        reply.generators.emplace_back(std::make_unique<model::generator>(*gen));

        return reply;
    }

    return to_error(api::error_type::NOT_FOUND);
}

reply_msg server::handle_request(const request::generator::create& request)
{
    // If user did not specify an id create one for them.
    auto& src = request.source;
    if (src->id().empty())
        src->id(core::to_string(core::uuid::random()));
    else if (!config::op_config_validate_id_string(src->id()))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    auto result = m_generator_stack.create(*src);
    if (!result) return to_error(error_type::CUSTOM_ERROR, 0, result.error());

    auto reply = reply::generator::list{};
    reply.generators.emplace_back(
        std::make_unique<model::generator>(*result.value()));

    return reply;
}

reply_msg server::handle_request(const request::generator::erase& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.erase(request.id))
        return to_error(api::error_type::NOT_FOUND);

    return reply::ok{};
}

reply_msg
server::handle_request(const request::generator::bulk::create& request)
{
    auto reply = reply::generator::list{};

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

reply_msg server::handle_request(const request::generator::bulk::erase& request)
{
    bool failed = false;
    for (const auto& id : request.ids) {
        failed = !m_generator_stack.erase(id) || failed;
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some generators from the list cannot be deleted");

    return api::reply::ok{};
}

reply_msg server::handle_request(const request::generator::start& request)
{
    if (!m_generator_stack.generator(request.id))
        return to_error(api::error_type::NOT_FOUND);

    try {
        auto result = m_generator_stack.start_generator(
            request.id, request.dynamic_results);
        if (!result) throw std::logic_error(result.error());

        auto reply = reply::statistic::list{};
        reply.results.emplace_back(
            std::make_unique<model::generator_result>(result.value()));
        return reply;
    } catch (const std::logic_error& e) {
        return to_error(error_type::CUSTOM_ERROR, 0, e.what());
    }
}

reply_msg server::handle_request(const request::generator::stop& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.stop_generator(request.id))
        return to_error(api::error_type::NOT_FOUND);

    return reply::ok{};
}

reply_msg server::handle_request(const request::generator::bulk::start& request)
{
    for (const auto& id : request.ids)
        if (!m_generator_stack.generator(id))
            return to_error(api::error_type::NOT_FOUND,
                            0,
                            "Generator from the list with ID '" + id
                                + "' was not found");

    using string_pair = std::pair<std::string, std::string>;
    std::forward_list<string_pair> not_runned_before;
    auto rollback = [&not_runned_before, this]() {
        for (const auto& pair : not_runned_before) {
            m_generator_stack.stop_generator(pair.first);
            m_generator_stack.erase_statistics(pair.second);
        }
    };

    try {
        auto reply = reply::statistic::list{};
        for (const auto& id : request.ids) {
            auto gen = m_generator_stack.generator(id);
            if (gen->running()) continue;

            auto stats =
                m_generator_stack.start_generator(id, request.dynamic_results);
            if (!stats) throw std::logic_error(stats.error());

            not_runned_before.push_front(std::make_pair(id, stats->id()));

            reply.results.emplace_back(
                std::make_unique<model::generator_result>(stats.value()));
        }
        return reply;
    } catch (const std::logic_error& e) {
        rollback();
        return to_error(error_type::CUSTOM_ERROR, 0, e.what());
    }
}

reply_msg server::handle_request(const request::generator::bulk::stop& request)
{
    for (const auto& id : request.ids)
        if (!m_generator_stack.generator(id))
            return to_error(api::error_type::NOT_FOUND,
                            0,
                            "Generator from the list with ID '" + id
                                + "' was not found");

    for (const auto& id : request.ids) m_generator_stack.stop_generator(id);

    return api::reply::ok{};
}

reply_msg server::handle_request(const request::generator::toggle& request)
{
    auto out_gen = m_generator_stack.generator(request.ids->first);
    auto in_gen = m_generator_stack.generator(request.ids->second);
    if (!out_gen || !in_gen) { return to_error(api::error_type::NOT_FOUND); }

    if (!out_gen->running() || in_gen->running()) {
        return to_error(
            error_type::CUSTOM_ERROR,
            0,
            "Generator(s) not in correct state for toggle operation.");
    }

    try {
        auto result = m_generator_stack.toggle_generator(
            request.ids->first, request.ids->second, request.dynamic_results);
        if (!result) throw std::logic_error(result.error());

        auto reply = reply::statistic::list{};
        reply.results.emplace_back(
            std::make_unique<model::generator_result>(result.value()));
        return reply;
    } catch (const std::logic_error& e) {
        return to_error(error_type::CUSTOM_ERROR, 0, e.what());
    }
}

reply_msg server::handle_request(const request::statistic::list&)
{
    auto reply = reply::statistic::list{};

    auto list = m_generator_stack.list_statistics();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply.results),
                   [](const auto& i) {
                       return std::make_unique<model::generator_result>(i);
                   });

    return reply;
}

reply_msg server::handle_request(const request::statistic::get& request)
{
    if (auto stat = m_generator_stack.statistics(request.id); stat) {
        auto reply = reply::statistic::list{};
        reply.results.emplace_back(
            std::make_unique<model::generator_result>(stat.value()));

        return reply;
    }

    return to_error(api::error_type::NOT_FOUND);
}

reply_msg server::handle_request(const request::statistic::erase& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_generator_stack.statistics(request.id))
        return to_error(api::error_type::NOT_FOUND);

    if (!m_generator_stack.erase_statistics(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "Statistics are active");

    return reply::ok{};
}

reply_msg server::handle_request(const request::server::list&)
{
    auto reply = reply::server::list{};

    auto list = m_server_stack.list();
    std::transform(list.begin(),
                   list.end(),
                   std::back_inserter(reply.servers),
                   [](const auto& i) {
                       return std::make_unique<model::server>(
                           model::server(*i));
                   });

    return reply;
}

reply_msg server::handle_request(const request::server::get& request)
{
    if (auto gen = m_server_stack.server(request.id); gen) {
        auto reply = reply::server::list{};
        reply.servers.emplace_back(std::make_unique<model::server>(*gen));

        return reply;
    }

    return to_error(api::error_type::NOT_FOUND);
}

reply_msg server::handle_request(const request::server::create& request)
{
    // If user did not specify an id create one for them.
    auto& src = request.source;
    if (src->id().empty())
        src->id(core::to_string(core::uuid::random()));
    else if (!config::op_config_validate_id_string(src->id()))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    auto list = m_server_stack.list();
    auto r = std::find_if(list.begin(), list.end(), [&](const auto& g) {
        return g->port() == src->port();
    });
    if (r != std::end(list)) {
        return to_error(error_type::CUSTOM_ERROR, 0, "Port is busy");
    }

    auto result = m_server_stack.create(*src);
    if (!result) return to_error(error_type::CUSTOM_ERROR, 0, result.error());

    auto reply = reply::server::list{};
    reply.servers.emplace_back(
        std::make_unique<model::server>(*result.value()));

    return reply;
}

reply_msg server::handle_request(const request::server::erase& request)
{
    if (!config::op_config_validate_id_string(request.id))
        return to_error(error_type::CUSTOM_ERROR, 0, "ID is not valid");

    if (!m_server_stack.erase(request.id))
        return to_error(api::error_type::NOT_FOUND);

    return reply::ok{};
}

reply_msg server::handle_request(const request::server::bulk::create& request)
{
    auto reply = reply::server::list{};

    auto remove_created_items = [&]() -> auto
    {
        for (const auto& item : reply.servers) {
            m_server_stack.erase(item->id());
        }
    };

    for (const auto& source : request.servers) {
        // If user did not specify an id create one for them.
        if (source->id().empty()) {
            source->id(core::to_string(core::uuid::random()));
        }

        if (auto id_check = config::op_config_validate_id_string(source->id());
            !id_check) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));
        }

        auto result = m_server_stack.create(*source);
        if (!result) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, result.error()));
        }
        reply.servers.emplace_back(
            std::make_unique<model::server>(*result.value()));
    }

    return reply;
}

reply_msg server::handle_request(const request::server::bulk::erase& request)
{
    bool failed = false;
    for (const auto& id : request.ids) {
        failed = !m_server_stack.erase(id) || failed;
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some generators from the list cannot be deleted");

    return api::reply::ok{};
}

static int _handle_rpc_request(const op_event_data* data, void* arg)
{
    auto* s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = openperf::message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> reply_msg {
            return s->handle_request(request);
        };
        auto reply = std::visit(request_visitor, *request);
        if (openperf::message::send(data->socket,
                                    serialize_reply(std::move(reply)))
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
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
{
    struct op_event_callbacks callbacks = {.on_read = _handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

} // namespace openperf::network::api
