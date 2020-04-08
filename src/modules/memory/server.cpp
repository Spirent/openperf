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
    for (auto id : generator_stack->ids()) {
        const auto& gnr = generator_stack->generator(id);
        reply::generator::item item{
            {.id = id}, .is_running = gnr.is_running(), .config = gnr.config()};
        list.push_back(item);
    }

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
    reply::generator::bulk::list reply;
    for (auto id : req) {
        if (!generator_stack->contains(id)) {
            reply.push_back(reply::generator::bulk::result{
                id, reply::generator::bulk::result::NOT_FOUND});
            continue;
        }

        auto& gnr = generator_stack->generator(id);

        gnr.start();
        gnr.resume();

        reply.push_back(reply::generator::bulk::result{
            id,
            (gnr.is_running()) ? reply::generator::bulk::result::SUCCESS
                               : reply::generator::bulk::result::FAIL});
    }

    return reply;
}

api_reply server::handle_request(const request::generator::bulk::stop& req)
{
    reply::generator::bulk::list reply;
    for (auto id : req) {
        if (!generator_stack->contains(id)) {
            reply.push_back(reply::generator::bulk::result{
                id, reply::generator::bulk::result::NOT_FOUND});
            continue;
        }

        auto& gnr = generator_stack->generator(id);

        gnr.pause();

        reply.push_back(reply::generator::bulk::result{
            id,
            (gnr.is_running()) ? reply::generator::bulk::result::FAIL
                               : reply::generator::bulk::result::SUCCESS});
    }

    return reply;
}

api_reply server::handle_request(const request::statistic::list&)
{
    reply::statistic::list list;
    for (auto id : generator_stack->ids()) {
        reply::statistic::item item{{.id = id},
                                    generator_stack->generator(id).stat()};

        list.push_back(item);
    }

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
