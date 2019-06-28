#include <cerrno>

#include "packetio/internal_api.h"
#include "packetio/internal_client.h"

namespace icp::packetio::internal::api {

static tl::expected<reply_msg, int> do_request(void* socket,
                                               const request_msg& request)
{
    if (send_message(socket, serialize_request(request)) != 0) {
        return (tl::make_unexpected(errno));
    }

    return (recv_message(socket).and_then(deserialize_reply));
}

client::client(void* context)
    : m_socket(icp_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{}

tl::expected<std::string, int> client::add_task(std::string_view name,
                                                workers::context ctx,
                                                event_loop::event_notifier notify,
                                                event_loop::callback_function callback,
                                                std::any arg)
{
    auto request = request_task_add {
        .task = {
            .ctx = ctx,
            .notifier = notify,
            .callback = callback,
            .arg = arg
        }
    };

    if (name.length() > name_length_max) {
        ICP_LOG(ICP_LOG_WARNING, "Truncating task name to %.*s\n",
                static_cast<int>(name_length_max), name.data());
    }

    std::copy(name.data(),
              name.data() + std::min(name.length(), name_length_max),
              request.task.name);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) {
        return (tl::make_unexpected(reply.error()));
    }

    if (auto success = std::get_if<reply_task_add>(&reply.value())) {
        return (success->task_id);
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int> client::del_task(std::string_view task_id)
{
    auto request = request_task_del {
        .task_id = std::string(task_id)
    };

    auto reply = do_request(m_socket.get(), request);
    if (!reply) {
        return (tl::make_unexpected(reply.error()));
    }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

}
