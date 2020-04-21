#include <zmq.h>

#include "block/api.hpp"
#include "block/server.hpp"
#include "config/op_config_utils.hpp"
#include "block/block_generator.hpp"
#include "models/device.hpp"
#include "models/file.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::block::api {

using namespace openperf::block::generator;

reply_msg server::handle_request(const request_block_device_list&)
{
    auto reply = reply_block_devices{};
    for (auto device : m_device_stack->block_devices_list()) {
        reply.devices.emplace_back(std::make_unique<model::device>(*device));
    }
    return reply;
}

reply_msg server::handle_request(const request_block_device& request)
{
    auto reply = reply_block_devices{};
    auto blkdev = m_device_stack->get_block_device(request.id);

    if (!blkdev) { return to_error(api::error_type::NOT_FOUND); }
    reply.devices.emplace_back(std::make_unique<model::device>(*blkdev));

    return reply;
}

reply_msg server::handle_request(const request_block_file_list&)
{
    auto reply = reply_block_files{};
    for (auto blkfile : m_file_stack->files_list()) {
        reply.files.emplace_back(std::make_unique<model::file>(*blkfile));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_file& request)
{
    auto reply = reply_block_files{};
    auto blkfile = m_file_stack->get_block_file(request.id);

    if (!blkfile) { return to_error(api::error_type::NOT_FOUND); }
    reply.files.emplace_back(std::make_unique<model::file>(*blkfile));

    return reply;
}

reply_msg server::handle_request(const request_block_file_add& request)
{
    // If user did not specify an id create one for them.
    if (request.source->get_id().empty()) {
        request.source->set_id(core::to_string(core::uuid::random()));
    }

    if (auto id_check = config::op_config_validate_id_string(request.source->get_id()); !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));

    auto result = m_file_stack->create_block_file(*request.source);
    if (!result) {
        return to_error(error_type::CUSTOM_ERROR, 0, result.error());
    }

    auto reply = reply_block_files{};
    reply.files.emplace_back(std::make_unique<model::file>(*result.value()));
    return reply;
}

reply_msg server::handle_request(const request_block_file_del& request)
{
    if (m_file_stack->delete_block_file(request.id)) {
        return reply_ok{};
    } else {
        return to_error(api::error_type::NOT_FOUND);
    }
}

reply_msg server::handle_request(const request_block_generator_list&)
{
    auto reply = reply_block_generators{};
    for (auto blkgenerator : m_generator_stack->block_generators_list()) {
        reply.generators.emplace_back(std::make_unique<model::block_generator>(*blkgenerator));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator& request)
{
    auto reply = reply_block_generators{};
    auto blkgenerator = m_generator_stack->get_block_generator(request.id);

    if (!blkgenerator) { return to_error(api::error_type::NOT_FOUND); }
    reply.generators.emplace_back(std::make_unique<model::block_generator>(*blkgenerator));

    return reply;
}

reply_msg server::handle_request(const request_block_generator_add& request)
{
    // If user did not specify an id create one for them.
    if (request.source->get_id().empty()) {
        request.source->set_id(core::to_string(core::uuid::random()));
    }

    if (auto id_check = config::op_config_validate_id_string(request.source->get_id()); !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));

    auto result = m_generator_stack->create_block_generator(*request.source, {m_file_stack.get(), m_device_stack.get()});
    if (!result) { return (to_error(error_type::CUSTOM_ERROR, 0, result.error())); }
    auto reply = reply_block_generators{};
    reply.generators.emplace_back(std::make_unique<model::block_generator>(*result.value()));
    return reply;
}

reply_msg server::handle_request(const request_block_generator_del& request)
{
    if (m_generator_stack->delete_block_generator(request.id)) {
        return reply_ok{};
    } else {
        return to_error(api::error_type::NOT_FOUND);
    }
}

reply_msg server::handle_request(const request_block_generator_start& request)
{
    auto result = m_generator_stack->start_generator(request.id);

    if (!result) { return to_error(api::error_type::CUSTOM_ERROR, 0, result.error()); }

    if (!result.value()) {
        return to_error(api::error_type::NOT_FOUND);
    }

    auto reply = reply_block_generator_results{};
    reply.results.emplace_back(std::make_unique<model::block_generator_result>(*result.value()));
    return reply;
}

reply_msg server::handle_request(const request_block_generator_stop& request)
{
    if (m_generator_stack->stop_generator(request.id)) {
        return reply_ok{};
    } else {
        return to_error(api::error_type::NOT_FOUND);
    }
}

reply_msg
server::handle_request(const request_block_generator_bulk_start& request)
{
    auto reply = reply_block_generator_results{};

    for (auto& id : request.ids)
    {
        auto stats = m_generator_stack->start_generator(*id);
        if (!stats || !stats.value()) {
            for (auto& stat : reply.results) {
                m_generator_stack->stop_generator(stat->get_generator_id());
                m_generator_stack->delete_statistics(stat->get_id());
            }
            if (!stats) {
                return to_error(api::error_type::CUSTOM_ERROR, 0, "Generator " + *id + " not found");
            } else {
                return to_error(api::error_type::CUSTOM_ERROR, 0, stats.error());
            }
        }

        auto reply_model = model::block_generator_result(*stats.value());
        reply.results.emplace_back(std::make_unique<model::block_generator_result>(reply_model));
    }

     return reply;
}

reply_msg
server::handle_request(const request_block_generator_bulk_stop& request)
{
    bool failed = false;
    for (auto& id : request.ids) {
        failed = failed || !m_generator_stack->stop_generator(*id);
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR, 0, "Some generators from the list were not found");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_result_list&)
{
    auto reply = reply_block_generator_results{};
    for (auto stat : m_generator_stack->list_statistics()) {
        reply.results.emplace_back(std::make_unique<model::block_generator_result>(*stat));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator_result& request)
{
    auto reply = reply_block_generator_results{};
    auto stat = m_generator_stack->get_statistics(request.id);

    if (!stat) { return to_error(api::error_type::NOT_FOUND); }
    reply.results.emplace_back(std::make_unique<model::block_generator_result>(*stat));

    return reply;
}

reply_msg
server::handle_request(const request_block_generator_result_del& request)
{
    if (m_generator_stack->delete_statistics(request.id)) {
        return reply_ok{};
    } else {
        return to_error(api::error_type::NOT_FOUND);
    }
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
    , m_device_stack(std::make_unique<device_stack>())
    , m_file_stack(std::make_unique<file_stack>())
    , m_generator_stack(std::make_unique<generator_stack>())
{

    struct op_event_callbacks callbacks = {.on_read = _handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

} // namespace openperf::block::api
