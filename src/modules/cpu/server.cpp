#include <zmq.h>

#include "api.hpp"
#include "server.hpp"
#include "config/op_config_utils.hpp"

namespace openperf::cpu::api {

/*
reply_msg server::handle_request(const request_block_generator_list&)
{
    auto reply = reply_block_generators{};
    for (auto blkgenerator : blk_generator_stack->block_generators_list()) {
        reply.generators.emplace_back(api::to_api_model(*blkgenerator));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator& request)
{
    auto reply = reply_block_generators{};
    auto blkgenerator = blk_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return to_error(api::error_type::NOT_FOUND); }
    reply.generators.emplace_back(api::to_api_model(*blkgenerator));

    return reply;
}

reply_msg server::handle_request(const request_block_generator_add& request)
{
    if (auto id_check = config::op_config_validate_id_string(request.source.id);
        !id_check)
        return (to_error(error_type::EAI_ERROR));

    auto block_generator_model = api::from_api_model(request.source);
    // If user did not specify an id create one for them.kjuolpik
    if (block_generator_model->get_id() == api::empty_id_string) {
        block_generator_model->set_id(core::to_string(core::uuid::random()));
    }

    auto result = blk_generator_stack->create_block_generator(
        *block_generator_model, {blk_file_stack.get(), blk_device_stack.get()});
    if (!result) { return to_error(error_type::NOT_FOUND); }
    auto reply = reply_block_generators{};
    reply.generators.emplace_back(api::to_api_model(*result.value()));
    return reply;
}

reply_msg server::handle_request(const request_block_generator_del& request)
{
    blk_generator_stack->delete_block_generator(request.id);
    return reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_start& request)
{
    auto blkgenerator = blk_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return to_error(api::error_type::NOT_FOUND); }
    blkgenerator->start();

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_stop& request)
{
    auto blkgenerator = blk_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return api::reply_ok{}; }
    blkgenerator->stop();

    return api::reply_ok{};
}

reply_msg
server::handle_request(const request_block_generator_bulk_start& request)
{
    auto reply = api::reply_block_generator_bulk_start{};

    for (auto& req : request.ids) {
        auto blkgenerator = blk_generator_stack->get_block_generator(req.id);
        if (!blkgenerator) {
            auto fg = api::failed_generator{};
            memcpy(fg.id, req.id, sizeof(req.id));
            fg.err = to_error(api::error_type::NOT_FOUND).info;
            reply.failed.push_back(fg);
            continue;
        }
        blkgenerator->start();
        auto fg = api::failed_generator{};
        memcpy(fg.id, req.id, sizeof(req.id));
        fg.err = to_error(api::error_type::NONE).info;
        reply.failed.push_back(fg);
    }
    return reply;
}

reply_msg
server::handle_request(const request_block_generator_bulk_stop& request)
{
    for (auto& id : request.ids) {
        auto blkgenerator = blk_generator_stack->get_block_generator(id.id);
        if (!blkgenerator) { continue; }
        blkgenerator->stop();
    }
    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_result_list&)
{
    auto reply = reply_block_generator_results{};
    for (auto blkgenerator : blk_generator_stack->block_generators_list()) {
        reply.results.emplace_back(
            api::to_api_model(*blkgenerator->get_statistics()));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator_result& request)
{
    auto reply = reply_block_generator_results{};
    auto blkgenerator = blk_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return to_error(api::error_type::NOT_FOUND); }
    reply.results.emplace_back(
        api::to_api_model(*blkgenerator->get_statistics()));

    return reply;
}

reply_msg
server::handle_request(const request_block_generator_result_del& request)
{
    auto blkgenerator = blk_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return reply_ok{}; }
    blkgenerator->clear_statistics();

    return reply_ok{};
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
        if (send_message(data->socket, serialize_reply(reply)) == -1) {
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
    , blk_device_stack(std::make_unique<device_stack>())
    , blk_file_stack(std::make_unique<file_stack>())
    , blk_generator_stack(std::make_unique<generator_stack>())
{

    struct op_event_callbacks callbacks = {.on_read = _handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}*/

} // namespace openperf::cpu::api
