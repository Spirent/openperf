#include <cerrno>

#include "packetio/internal_api.hpp"
#include "packetio/internal_client.hpp"

namespace openperf::packetio::internal::api {

static tl::expected<reply_msg, int> do_request(void* socket,
                                               const request_msg& request)
{
    if (send_message(socket, serialize_request(request)) != 0) {
        return (tl::make_unexpected(errno));
    }

    return (recv_message(socket).and_then(deserialize_reply));
}

client::client(void* context)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{}

client::client(client&& other) noexcept
    : m_socket(std::move(other.m_socket))
{}

client& client::operator=(client&& other) noexcept
{
    if (this != &other) { m_socket = std::move(other.m_socket); }
    return (*this);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_rx_ids(std::optional<std::string_view> obj_id)
{
    auto request = request_worker_rx_ids{};
    if (obj_id) { request.object_id = std::string(*obj_id); }

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    return (std::get<reply_worker_ids>(reply.value()).worker_ids);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_tx_ids(std::optional<std::string_view> obj_id)
{
    auto request = request_worker_tx_ids{};
    if (obj_id) { request.object_id = std::string(*obj_id); }

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    return (std::get<reply_worker_ids>(reply.value()).worker_ids);
}

tl::expected<void, int> client::add_sink(std::string_view src_id,
                                         packet::generic_sink sink)
{
    if (src_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Source ID, %.*s, is too big\n",
               static_cast<int>(src_id.length()),
               src_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_sink_add{.data = {.sink = std::move(sink)}};
    std::copy_n(src_id.data(), src_id.length(), request.data.src_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int> client::del_sink(std::string_view src_id,
                                         packet::generic_sink sink)
{
    if (src_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Source ID, %.*s, is too big\n",
               static_cast<int>(src_id.length()),
               src_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_sink_del{.data = {.sink = std::move(sink)}};
    std::copy_n(src_id.data(), src_id.length(), request.data.src_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int> client::add_source(std::string_view dst_id,
                                           packet::generic_source source)
{
    if (dst_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Destination ID, %.*s, is too big\n",
               static_cast<int>(dst_id.length()),
               dst_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_source_add{.data = {.source = std::move(source)}};
    std::copy_n(dst_id.data(), dst_id.length(), request.data.dst_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int> client::del_source(std::string_view dst_id,
                                           packet::generic_source source)
{
    if (dst_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Destination ID, %.*s, is too big\n",
               static_cast<int>(dst_id.length()),
               dst_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_source_del{.data = {.source = std::move(source)}};
    std::copy_n(dst_id.data(), dst_id.length(), request.data.dst_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<std::string, int>
client::add_task_impl(workers::context ctx,
                      std::string_view name,
                      event_loop::event_notifier notify,
                      event_loop::event_handler&& on_event,
                      std::optional<event_loop::delete_handler>&& on_delete,
                      std::any&& arg)
{
    auto request = request_task_add{
        .data = {.ctx = ctx,
                 .notifier = notify,
                 .on_event = std::forward<decltype(on_event)>(on_event),
                 .on_delete = std::forward<decltype(on_delete)>(on_delete),
                 .arg = std::forward<std::any>(arg)}};

    if (name.length() > name_length_max) {
        OP_LOG(OP_LOG_WARNING,
               "Truncating task name to %.*s\n",
               static_cast<int>(name_length_max),
               name.data());
    }

    std::copy(name.data(),
              name.data() + std::min(name.length(), name_length_max),
              request.data.name);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_task_add>(&reply.value())) {
        return (success->task_id);
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<std::string, int>
client::add_task(workers::context ctx,
                 std::string_view name,
                 event_loop::event_notifier notify,
                 event_loop::event_handler on_event,
                 std::any arg)
{
    return (add_task_impl(
        ctx, name, notify, std::move(on_event), std::nullopt, std::move(arg)));
}

tl::expected<std::string, int>
client::add_task(workers::context ctx,
                 std::string_view name,
                 event_loop::event_notifier notify,
                 event_loop::event_handler on_event,
                 event_loop::delete_handler on_delete,
                 std::any arg)
{
    return (add_task_impl(ctx,
                          name,
                          notify,
                          std::move(on_event),
                          std::move(on_delete),
                          std::move(arg)));
}

tl::expected<void, int> client::del_task(std::string_view task_id)
{
    auto request = request_task_del{.task_id = std::string(task_id)};

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

} // namespace openperf::packetio::internal::api
