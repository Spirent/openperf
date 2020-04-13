#include "memory/server.hpp"
#include "memory/api.hpp"
#include "memory/info.hpp"

namespace openperf::memory::api {

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
    , generator_stack(std::make_unique<memory::generator_collection>())
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

        if (send_message(data->socket, serialize(reply)) == -1) {
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
    reply::generator::list list;
    auto ids = generator_stack->ids();
    std::transform(
        ids.begin(), ids.end(), std::back_inserter(list), [&](auto id) -> auto {
            const auto& gnr = generator_stack->generator(id);
            return reply::generator::item{{.id = id},
                                          .is_running = gnr.is_running(),
                                          .config = gnr.config()};
        });

    return list;
}

api_reply server::handle_request(const request::generator::get& req)
{
    if (generator_stack->contains(req.id)) {
        auto& gnr = generator_stack->generator(req.id);

        reply::generator::item reply{
            req, .is_running = gnr.is_running(), .config = gnr.config()};
        return reply;
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::erase& req)
{
    if (generator_stack->contains(req.id)) {
        generator_stack->erase(req.id);
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::create& req)
{
    try {
        auto id = generator_stack->create(req.id, req.config);
        auto& gnr = generator_stack->generator(id);

        if (req.is_running) {
            gnr.resume();
            gnr.start();
        }

        auto reply = reply::generator::item{
            {.id = id}, .is_running = gnr.is_running(), .config = gnr.config()};
        return reply;
    } catch (std::invalid_argument e) {
        return reply::error{.type = reply::error::EXISTS};
    } catch (std::domain_error e) {
        return reply::error{.type = reply::error::INVALID_ID};
    }
}

api_reply server::handle_request(const request::generator::stop& req)
{
    if (generator_stack->contains(req.id)) {
        generator_stack->generator(req.id).pause();
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::start& req)
{
    if (generator_stack->contains(req.id)) {
        generator_stack->generator(req.id).resume();
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::generator::bulk::start& req)
{
    for (auto id : req) {
        if (!generator_stack->contains(id))
            return reply::error{.type = reply::error::NOT_FOUND};
    }

    std::forward_list<std::string> not_runned_before;
    for (auto id : req) {
        auto& generator = generator_stack->generator(id);
        if (!generator.is_running()) not_runned_before.emplace_front(id);

        generator.start();
        generator.resume();
        if (!generator.is_running()) {
            for (auto& rollback_id : not_runned_before) {
                generator_stack->generator(rollback_id).pause();
            }
            return reply::error{};
        }
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::generator::bulk::stop& req)
{
    for (auto id : req) {
        if (!generator_stack->contains(id))
            return reply::error{.type = reply::error::NOT_FOUND};
    }

    std::forward_list<std::string> runned_before;
    for (auto id : req) {
        auto& generator = generator_stack->generator(id);
        if (generator.is_running()) runned_before.emplace_front(id);

        generator.pause();
        if (generator.is_running()) {
            for (auto& rollback_id : runned_before) {
                generator_stack->generator(rollback_id).resume();
            }
            return reply::error{};
        }
    }

    return reply::ok{};
}

api_reply server::handle_request(const request::statistic::list&)
{
    auto ids = generator_stack->ids();
    reply::statistic::list list;
    std::transform(
        ids.begin(), ids.end(), std::back_inserter(list), [&](auto id) -> auto {
            return reply::statistic::item{
                {.id = id}, generator_stack->generator(id).stat()};
        });

    return list;
}

api_reply server::handle_request(const request::statistic::get& req)
{
    if (generator_stack->contains(req.id)) {
        return reply::statistic::item{
            req, generator_stack->generator(req.id).stat()};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::statistic::erase& req)
{
    if (generator_stack->contains(req.id)) {
        generator_stack->generator(req.id).clear_stat();
        return reply::ok{};
    }

    return reply::error{.type = reply::error::NOT_FOUND};
}

api_reply server::handle_request(const request::info&)
{
    return reply::info{memory_info::get()};
}

} // namespace openperf::memory::api
