#include <cerrno>

#include "message/serialized_message.hpp"
#include "packetio/internal_api.hpp"
#include "packetio/internal_client.hpp"

namespace openperf::packetio::internal::api {

static tl::expected<reply_msg, int> do_request(void* socket,
                                               request_msg&& request)
{
    if (auto error = message::send(
            socket, serialize_request(std::forward<request_msg>(request)));
        error != 0) {
        return tl::make_unexpected(error);
    }

    auto reply = message::recv(socket).and_then(deserialize_reply);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    return (std::move(*reply));
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

tl::expected<std::vector<unsigned>, int> client::get_worker_ids()
{
    return get_worker_ids(packet::traffic_direction::RXTX, std::nullopt);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_ids(packet::traffic_direction direction)
{
    return get_worker_ids(direction, std::nullopt);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_ids(packet::traffic_direction direction,
                       std::optional<std::string_view> obj_id)
{
    auto request = request_worker_ids{.direction = direction};
    if (obj_id) { request.object_id = std::string(*obj_id); }

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    return (std::get<reply_worker_ids>(reply.value()).worker_ids);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_rx_ids(std::optional<std::string_view> obj_id)
{
    return get_worker_ids(packet::traffic_direction::RX, obj_id);
}

tl::expected<std::vector<unsigned>, int>
client::get_worker_tx_ids(std::optional<std::string_view> obj_id)
{
    return get_worker_ids(packet::traffic_direction::TX, obj_id);
}

tl::expected<int, int> client::get_port_index(std::string_view port_id)
{
    auto request = request_port_index{std::string(port_id)};
    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto port_index = std::get_if<reply_port_index>(&reply.value())) {
        return (port_index->index);
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<workers::transmit_function, int>
client::get_transmit_function(std::string_view port_id)
{
    auto request = request_transmit_function{std::string(port_id)};
    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto transmit_function =
            std::get_if<reply_transmit_function>(&reply.value())) {
        return (transmit_function->f);
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int>
client::add_interface(std::string_view port_id,
                      interface::generic_interface interface)
{
    if (port_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Port ID, %.*s, is too big\n",
               static_cast<int>(port_id.length()),
               port_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request =
        request_interface_add{.data = {.interface = std::move(interface)}};
    std::copy_n(port_id.data(), port_id.length(), request.data.port_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int>
client::del_interface(std::string_view port_id,
                      interface::generic_interface interface)
{
    if (port_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Port ID, %.*s, is too big\n",
               static_cast<int>(port_id.length()),
               port_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request =
        request_interface_del{.data = {.interface = std::move(interface)}};
    std::copy_n(port_id.data(), port_id.length(), request.data.port_id);

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto success = std::get_if<reply_ok>(&reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<interface::generic_interface, int>
client::interface(std::string_view interface_id)
{
    if (interface_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Interface ID, %.*s, is too big\n",
               static_cast<int>(interface_id.length()),
               interface_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_interface{.interface_id = std::string(interface_id)};

    auto reply = do_request(m_socket.get(), request);
    if (!reply) { return (tl::make_unexpected(reply.error())); }

    if (auto maybe_interface = std::get_if<reply_interface>(&reply.value())) {
        return (maybe_interface->data.interface);
    } else if (auto error = std::get_if<reply_error>(&reply.value())) {
        return (tl::make_unexpected(error->value));
    }

    return (tl::make_unexpected(EBADMSG));
}

tl::expected<void, int> client::add_sink(packet::traffic_direction direction,
                                         std::string_view src_id,
                                         packet::generic_sink sink)
{
    if (src_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Source ID, %.*s, is too big\n",
               static_cast<int>(src_id.length()),
               src_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_sink_add{
        .data = {.direction = direction, .sink = std::move(sink)}};
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

tl::expected<void, int> client::del_sink(packet::traffic_direction direction,
                                         std::string_view src_id,
                                         packet::generic_sink sink)
{
    if (src_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Source ID, %.*s, is too big\n",
               static_cast<int>(src_id.length()),
               src_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_sink_del{
        .data = {.direction = direction, .sink = std::move(sink)}};
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

tl::expected<void, int> client::swap_source_impl(
    std::string_view dst_id,
    packet::generic_source&& outgoing,
    packet::generic_source&& incoming,
    std::optional<workers::source_swap_function>&& swap_function)
{
    if (dst_id.length() > name_length_max) {
        OP_LOG(OP_LOG_ERROR,
               "Destination ID, %.*s, is too big\n",
               static_cast<int>(dst_id.length()),
               dst_id.data());
        return (tl::make_unexpected(ENOMEM));
    }

    auto request = request_source_swap{
        .data = {.outgoing = std::forward<packet::generic_source>(outgoing),
                 .incoming = std::forward<packet::generic_source>(incoming),
                 .action =
                     std::forward<std::optional<workers::source_swap_function>>(
                         swap_function)}};
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

tl::expected<void, int> client::swap_source(std::string_view dst_id,
                                            packet::generic_source outgoing,
                                            packet::generic_source incoming)
{
    return (swap_source_impl(
        dst_id, std::move(outgoing), std::move(incoming), std::nullopt));
}

tl::expected<void, int>
client::swap_source(std::string_view dst_id,
                    packet::generic_source outgoing,
                    packet::generic_source incoming,
                    workers::source_swap_function&& swap_function)
{
    return (swap_source_impl(
        dst_id,
        std::move(outgoing),
        std::move(incoming),
        std::forward<workers::source_swap_function>(swap_function)));
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
