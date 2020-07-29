#include <zmq.h>

#include "block/api.hpp"
#include "block/server.hpp"
#include "config/op_config_utils.hpp"
#include "block/block_generator.hpp"
#include "message/serialized_message.hpp"
#include "models/device.hpp"
#include "models/file.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::block::api {

using namespace openperf::block::generator;

reply_msg server::handle_request(const request_block_device_list&)
{
    auto reply = reply_block_devices{};
    for (const auto& device : m_device_stack->block_devices_list()) {
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

reply_msg server::handle_request(const request_block_device_init& request)
{
    auto reply = reply_block_devices{};

    auto blkdev = m_device_stack->get_block_device(request.id);
    if (!blkdev) return to_error(api::error_type::NOT_FOUND);

    if (auto r = m_device_stack->initialize_device(request.id); !r)
        return to_error(api::error_type::CUSTOM_ERROR, 0, r.error());

    return reply_ok{};
}

reply_msg server::handle_request(const request_block_file_list&)
{
    auto reply = reply_block_files{};
    for (const auto& blkfile : m_file_stack->files_list()) {
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
    if (request.source->id().empty()) {
        request.source->id(core::to_string(core::uuid::random()));
    }

    if (auto id_check =
            config::op_config_validate_id_string(request.source->id());
        !id_check)
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
    auto result = m_file_stack->delete_block_file(request.id);

    if (!result) {
        switch (result.error()) {
        case block::file::deletion_error_type::NOT_FOUND:
            return to_error(api::error_type::NOT_FOUND);
        case block::file::deletion_error_type::BUSY:
            return to_error(error_type::CUSTOM_ERROR,
                            0,
                            "File \"" + request.id + "\" is busy");
        }
    }

    return reply_ok{};
}

reply_msg server::handle_request(const request_block_file_bulk_add& request)
{
    auto reply = reply_block_files{};

    auto remove_created_items = [&]() -> auto
    {
        for (const auto& item : reply.files) {
            m_file_stack->delete_block_file(item->id());
        }
    };

    for (const auto& source : request.files) {
        // If user did not specify an id create one for them.
        if (source->id().empty()) {
            source->id(core::to_string(core::uuid::random()));
        }

        if (auto id_check = config::op_config_validate_id_string(source->id());
            !id_check) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));
        }

        auto result = m_file_stack->create_block_file(*source);
        if (!result) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, result.error()));
        }
        reply.files.emplace_back(
            std::make_unique<model::file>(*result.value()));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_file_bulk_del& request)
{
    if (auto failed = std::count_if(
            request.ids.begin(),
            request.ids.end(),
            [&](auto& id) { return !m_file_stack->delete_block_file(*id); });
        failed > 0)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some files from the list cannot be deleted");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_list&)
{
    auto reply = reply_block_generators{};
    for (const auto& blkgenerator :
         m_generator_stack->block_generators_list()) {
        reply.generators.emplace_back(
            std::make_unique<model::block_generator>(*blkgenerator));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator& request)
{
    auto reply = reply_block_generators{};
    auto blkgenerator = m_generator_stack->block_generator(request.id);

    if (!blkgenerator) { return to_error(api::error_type::NOT_FOUND); }
    reply.generators.emplace_back(
        std::make_unique<model::block_generator>(*blkgenerator));

    return reply;
}

reply_msg server::handle_request(const request_block_generator_add& request)
{
    // If user did not specify an id create one for them.
    if (request.source->id().empty()) {
        request.source->id(core::to_string(core::uuid::random()));
    }

    if (auto id_check =
            config::op_config_validate_id_string(request.source->id());
        !id_check)
        return (to_error(error_type::CUSTOM_ERROR, 0, "Id is not valid"));

    auto result = m_generator_stack->create_block_generator(
        *request.source, {m_file_stack.get(), m_device_stack.get()});
    if (!result) {
        return (to_error(error_type::CUSTOM_ERROR, 0, result.error()));
    }
    auto reply = reply_block_generators{};
    reply.generators.emplace_back(
        std::make_unique<model::block_generator>(*result.value()));
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

reply_msg
server::handle_request(const request_block_generator_bulk_add& request)
{
    auto reply = reply_block_generators{};

    auto remove_created_items = [&]() -> auto
    {
        for (const auto& item : reply.generators) {
            m_generator_stack->delete_block_generator(item->id());
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

        auto result = m_generator_stack->create_block_generator(
            *source, {m_file_stack.get(), m_device_stack.get()});
        if (!result) {
            remove_created_items();
            return (to_error(error_type::CUSTOM_ERROR, 0, result.error()));
        }
        reply.generators.emplace_back(
            std::make_unique<model::block_generator>(*result.value()));
    }

    return reply;
}

reply_msg
server::handle_request(const request_block_generator_bulk_del& request)
{
    bool failed = false;
    for (const auto& id : request.ids) {
        failed = !m_generator_stack->delete_block_generator(*id) || failed;
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some generators from the list cannot be deleted");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_start& request)
{
    auto result = m_generator_stack->start_generator(request.id);

    if (!result) {
        return to_error(api::error_type::CUSTOM_ERROR, 0, result.error());
    }

    if (!result.value()) { return to_error(api::error_type::NOT_FOUND); }

    auto reply = reply_block_generator_results{};
    reply.results.emplace_back(
        std::make_unique<model::block_generator_result>(*result.value()));
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

    for (const auto& id : request.ids) {
        auto stats = m_generator_stack->start_generator(*id);
        if (!stats || !stats.value()) {
            for (const auto& stat : reply.results) {
                m_generator_stack->stop_generator(stat->generator_id());
                m_generator_stack->delete_statistics(stat->id());
            }
            if (!stats) {
                return to_error(api::error_type::CUSTOM_ERROR,
                                0,
                                "Generator " + *id + " not found");
            } else {
                return to_error(
                    api::error_type::CUSTOM_ERROR, 0, stats.error());
            }
        }

        auto reply_model = model::block_generator_result(*stats.value());
        reply.results.emplace_back(
            std::make_unique<model::block_generator_result>(reply_model));
    }

    return reply;
}

reply_msg
server::handle_request(const request_block_generator_bulk_stop& request)
{
    bool failed = false;
    for (const auto& id : request.ids) {
        failed = !m_generator_stack->stop_generator(*id) || failed;
    }
    if (failed)
        return to_error(api::error_type::CUSTOM_ERROR,
                        0,
                        "Some generators from the list were not found");

    return api::reply_ok{};
}

reply_msg server::handle_request(const request_block_generator_result_list&)
{
    auto reply = reply_block_generator_results{};
    for (const auto& stat : m_generator_stack->list_statistics()) {
        reply.results.emplace_back(
            std::make_unique<model::block_generator_result>(*stat));
    }

    return reply;
}

reply_msg server::handle_request(const request_block_generator_result& request)
{
    auto reply = reply_block_generator_results{};
    auto stat = m_generator_stack->statistics(request.id);

    if (!stat) { return to_error(api::error_type::NOT_FOUND); }
    reply.results.emplace_back(
        std::make_unique<model::block_generator_result>(*stat));

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
    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](const auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);
        if (message::send(data->socket, serialize_reply(std::move(reply)))
            == -1) {
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
