#include "message/serialized_message.hpp"
#include "server.hpp"
#include "api.hpp"
#include "info.hpp"

namespace openperf::memory::api {

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
    , m_generator_stack(std::make_unique<memory::generator_collection>())
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

api_reply server::handle_request(const request::generator::list&)
{
    auto id_to_config_transformer = [&](auto& id) -> auto
    {
        const auto& gnr = m_generator_stack->generator(id);
        return reply::generator::item{
            .id = id,
            .is_running = gnr.is_running(),
            .config = gnr.config(),
            .init_percent_complete = gnr.init_percent_complete(),
        };
    };

    reply::generator::list list{};
    auto ids = m_generator_stack->ids();
    std::transform(ids.begin(),
                   ids.end(),
                   std::back_inserter(list),
                   id_to_config_transformer);

    return list;
}

api_reply server::handle_request(const request::generator::get& req)
{
    if (m_generator_stack->contains(req.id)) {
        const auto& gnr = m_generator_stack->generator(req.id);

        return reply::generator::item{
            .id = req.id,
            .is_running = gnr.is_running(),
            .config = gnr.config(),
            .init_percent_complete = gnr.init_percent_complete(),
        };
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::erase& req)
{
    if (m_generator_stack->contains(req.id)) {
        m_generator_stack->erase(req.id);
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::create& req)
{
    try {
        auto id = m_generator_stack->create(req.id, req.config);

        const auto& gnr = m_generator_stack->generator(id);
        return reply::generator::item{
            .id = id,
            .is_running = gnr.is_running(),
            .config = gnr.config(),
            .init_percent_complete = gnr.init_percent_complete(),
        };
    } catch (const std::invalid_argument&) {
        return reply::error{.type = reply::error::EXISTS};
    } catch (const std::domain_error&) {
        return reply::error{.type = reply::error::INVALID_ID};
    } catch (const std::runtime_error& e) {
        return reply::error{
            .type = reply::error::CUSTOM,
            .message = e.what(),
        };
    }
}

api_reply server::handle_request(const request::generator::bulk::create& req)
{
    reply::generator::list list{};
    auto remove_created_items = [&]() {
        for (const auto& item : list) { m_generator_stack->erase(item.id); }
    };

    for (const auto& data : req) {
        try {
            auto id = m_generator_stack->create(data.id, data.config);
            const auto& gnr = m_generator_stack->generator(id);
            list.push_back(reply::generator::item{
                .id = id,
                .is_running = false,
                .config = gnr.config(),
                .init_percent_complete = gnr.init_percent_complete(),
            });
        } catch (const std::invalid_argument&) {
            remove_created_items();
            return reply::error{.type = reply::error::EXISTS};
        } catch (const std::domain_error&) {
            remove_created_items();
            return reply::error{.type = reply::error::INVALID_ID};
        } catch (const std::runtime_error& e) {
            remove_created_items();
            return reply::error{
                .type = reply::error::CUSTOM,
                .message = e.what(),
            };
        }
    }

    return list;
}

api_reply server::handle_request(const request::generator::bulk::erase& req)
{
    for (const auto& id : req) {
        if (!m_generator_stack->contains(id)) continue;

        m_generator_stack->erase(id);
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::generator::stop& req)
{
    if (m_generator_stack->contains(req.id)) {
        m_generator_stack->stop(req.id);
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::start& req)
{
    try {
        if (m_generator_stack->contains(req.id)) {
            m_generator_stack->start(req.id, req.dynamic_results);
            auto stat = m_generator_stack->stat(req.id);
            return reply::statistic::item{
                .id = stat.id,
                .generator_id = stat.generator_id,
                .stat = stat.stat,
                .dynamic_results = stat.dynamic_results,
            };
        }
    } catch (const std::runtime_error&) {
        return reply::error{.type = reply::error::NOT_INITIALIZED};
    } catch (const std::logic_error& e) {
        return reply::error{
            .type = reply::error::CUSTOM,
            .message = e.what(),
        };
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::bulk::start& req)
{
    if (std::any_of(req.ids.begin(), req.ids.end(), [&](const auto& id) {
            return (m_generator_stack->contains(id) == false);
        })) {
        return reply::error{.type = reply::error::NOT_FOUND};
    }

    auto stat_transformer = [](const auto& stat) {
        return reply::statistic::item{
            .id = stat.id,
            .generator_id = stat.generator_id,
            .stat = stat.stat,
            .dynamic_results = stat.dynamic_results,
        };
    };

    reply::statistic::list list{};
    std::forward_list<std::string> not_runned_before;
    try {
        for (const auto& id : req.ids) {
            const auto& gnr = m_generator_stack->generator(id);
            if (!gnr.is_running()) not_runned_before.emplace_front(id);

            m_generator_stack->start(id, req.dynamic_results);
            if (!gnr.is_running())
                throw std::logic_error("Generator with ID '" + id
                                       + "' start error.");

            list.push_back(stat_transformer(m_generator_stack->stat(id)));
        }
    } catch (const std::logic_error& e) {
        for (auto& rollback_id : not_runned_before)
            m_generator_stack->stop(rollback_id);

        return reply::error{
            .type = reply::error::CUSTOM,
            .message = e.what(),
        };
    }

    return list;
}

api_reply server::handle_request(const request::generator::bulk::stop& req)
{
    auto expr = std::any_of(req.begin(), req.end(), [&](const auto& id) {
        return (m_generator_stack->contains(id) == false);
    });

    if (expr) { return reply::error{.type = reply::error::NOT_FOUND}; }

    std::forward_list<std::string> runned_before;
    for (const auto& id : req) {
        auto& generator = m_generator_stack->generator(id);
        if (generator.is_running()) runned_before.emplace_front(id);

        m_generator_stack->stop(id);
        if (generator.is_running()) {
            for (auto& rollback_id : runned_before) {
                m_generator_stack->start(rollback_id);
            }
            return reply::error{};
        }
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::statistic::list&)
{
    auto stat_transformer = [](const auto& stat) {
        return reply::statistic::item{
            .id = stat.id,
            .generator_id = stat.generator_id,
            .stat = stat.stat,
            .dynamic_results = stat.dynamic_results,
        };
    };

    reply::statistic::list list{};
    auto stat_list = m_generator_stack->stats();
    std::transform(stat_list.begin(),
                   stat_list.end(),
                   std::back_inserter(list),
                   stat_transformer);

    return list;
}

api_reply server::handle_request(const request::statistic::get& req)
{
    if (m_generator_stack->contains_stat(req.id)) {
        auto stat = m_generator_stack->stat(req.id);
        return reply::statistic::item{
            .id = stat.id,
            .generator_id = stat.generator_id,
            .stat = stat.stat,
            .dynamic_results = stat.dynamic_results,
        };
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::statistic::erase& req)
{
    if (m_generator_stack->contains_stat(req.id)) {
        if (m_generator_stack->erase_stat(req.id)) return reply::ok{};

        return reply::error{.type = reply::error::ACTIVE_STAT};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::info&)
{
    return reply::info{memory_info::get()};
}

} // namespace openperf::memory::api
