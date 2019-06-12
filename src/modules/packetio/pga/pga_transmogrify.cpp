#include <cstring>
#include <optional>

#include <zmq.h>
#include <tl/expected.hpp>

#include "packetio/pga/pga_api.h"

namespace icp::packetio::pga::api {

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

template <typename VariantType, typename T, std::size_t index = 0>
constexpr size_t variant_index() {
    if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) {
        return (index);
    } else {
        return (variant_index<VariantType, T, index + 1>());
    }
}

struct serialized_request_message {
    zmq_msg_t type;
    zmq_msg_t id;
    zmq_msg_t item;
};

static void close(serialized_request_message& msg)
{
    zmq_msg_close(&msg.type);
    zmq_msg_close(&msg.id);
    zmq_msg_close(&msg.item);
}

struct serialized_reply_error {
    zmq_msg_t code;
    zmq_msg_t mesg;
};

struct serialized_reply_message {
    zmq_msg_t type;
    std::optional<serialized_reply_error> error;
};

static void close(serialized_reply_error& error)
{
    zmq_msg_close(&error.code);
    zmq_msg_close(&error.mesg);
}

static void close(serialized_reply_message& msg)
{
    zmq_msg_close(&msg.type);
    if (msg.error) close(*msg.error);
}

template <typename T>
static auto zmq_msg_init_from_value(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;

    return (0);
}

template <typename T>
static auto zmq_msg_init_from_buffer(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    std::copy(buffer, buffer + length, ptr);
    return (0);
}

static serialized_request_message to_serialized_message(const request_msg request)
{
    auto serialize_visitor = [&](const auto& msg_type) -> serialized_request_message {
                                 serialized_request_message msg;
                                 if (zmq_msg_init_from_value(&msg.type, request.index()) == -1
                                     || zmq_msg_init_from_buffer(&msg.id, msg_type.id.data(), msg_type.id.size()) == -1
                                     || zmq_msg_init_from_buffer(&msg.item,
                                                                 reinterpret_cast<const uint8_t*>(&msg_type.item),
                                                                 sizeof(msg_type.item))== -1) {
                                     close(msg);
                                     throw std::bad_alloc();
                                 }
                                 return (msg);
                             };

    return (std::visit(serialize_visitor, std::move(request)));
}

static serialized_reply_message to_serialized_message(const reply_msg reply)
{
    serialized_reply_message msg;

    std::visit(overloaded_visitor(
                   [&](const reply_ok&) {
                       if (zmq_msg_init_from_value(&msg.type, reply.index()) == -1) {
                           close(msg);
                           throw std::bad_alloc();
                       }
                   },
                   [&](const reply_error& error) {
                       msg.error = serialized_reply_error{};
                       if (zmq_msg_init_from_value(&msg.type, reply.index()) == -1
                           || zmq_msg_init_from_value(&msg.error->code, error.code) == -1
                           || zmq_msg_init_from_buffer(&msg.error->mesg,
                                                       error.mesg.data(),
                                                       error.mesg.size()) == -1) {
                           throw std::bad_alloc();
                       }
                   }), reply);

    return (msg);
}


static int send_message(void* socket, serialized_request_message&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.id, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.item, socket, 0) == -1) {
        close(msg);
        return (errno);
    }
    return (0);
}

static int send_message(void* socket, serialized_reply_message&& msg)
{
    if (zmq_msg_send(&msg.type, socket, (msg.error ? ZMQ_SNDMORE : 0)) == -1
        || (msg.error
            && (zmq_msg_send(&msg.error->code, socket, ZMQ_SNDMORE) == -1
                || zmq_msg_send(&msg.error->mesg, socket, 0) == -1))) {
        close(msg);
        return (errno);
    }

    return (0);
}

static std::optional<serialized_reply_error> recv_serialized_reply_error(void* socket, int flags)
{
    serialized_reply_error error;
    if (zmq_msg_init(&error.code) == -1
        || zmq_msg_init(&error.mesg) == -1) {
        return (std::nullopt);
    }

    if (zmq_msg_recv(&error.code, socket, flags) == -1
        || zmq_msg_recv(&error.mesg, socket, flags) == -1) {
        close(error);
        return (std::nullopt);
    }

    return (error);
}

static std::optional<reply_msg> recv_reply(void* socket, int flags = 0)
{
    zmq_msg_t reply_type __attribute__((cleanup(zmq_msg_close)));
    if (zmq_msg_init(&reply_type) == -1) {
        return (std::nullopt);
    }

    if (zmq_msg_recv(&reply_type, socket, flags) == -1) {
        return (std::nullopt);
    }

    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = *(reinterpret_cast<index_type*>(zmq_msg_data(&reply_type)));
    switch (idx) {
    case variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case variant_index<reply_msg, reply_error>(): {
        auto error = recv_serialized_reply_error(socket, flags);
        if (!error) return (std::nullopt);
        auto reply = reply_error{ .code = *reinterpret_cast<decltype(reply_error::code)*>(zmq_msg_data(&error->code)),
                                  .mesg = std::string(reinterpret_cast<char*>(zmq_msg_data(&error->mesg)),
                                                      zmq_msg_size(&error->mesg)) };
        close(*error);
        return (reply);
    }
    default:
        return (std::nullopt);
    }
}

static std::optional<serialized_request_message> recv_serialized_request(void* socket, int flags)
{
    serialized_request_message msg;

    if (zmq_msg_init(&msg.type) == -1
        || zmq_msg_init(&msg.id) == -1
        || zmq_msg_init(&msg.item) == -1) {
        return (std::nullopt);
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.id, socket, flags) == -1
        || zmq_msg_recv(&msg.item, socket, flags) == -1) {
        close(msg);
        return (std::nullopt);
    }

    return (msg);
}

template <typename T>
static T to_request(serialized_request_message& msg)
{
    return (T{ .id = std::string(reinterpret_cast<char*>(zmq_msg_data(&msg.id)),
                                zmq_msg_size(&msg.id)),
               .item = *(reinterpret_cast<decltype(T::item)*>(zmq_msg_data(&msg.item))) });
}

reply_msg submit_request(void* socket, const request_msg request)
{
    if (send_message(socket, to_serialized_message(std::move(request))) == -1) {
        return (reply_error{ .code = errno,
                             .mesg = std::string(zmq_strerror(errno)) });
    }

    auto reply = recv_reply(socket);
    if (!reply) {
        return (reply_error{ .code = errno,
                             .mesg = std::string(zmq_strerror(errno)) });
    }

    return (*reply);
}

std::optional<request_msg> recv_request(void* socket, int flags)
{
    auto msg = recv_serialized_request(socket, flags);
    if (!msg) return (std::nullopt);

    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(reinterpret_cast<index_type*>(zmq_msg_data(&msg->type)));
    switch (idx) {
    case variant_index<request_msg, request_add_interface_sink>():
        return (to_request<request_add_interface_sink>(*msg));
    case variant_index<request_msg, request_del_interface_sink>():
        return (to_request<request_del_interface_sink>(*msg));
    case variant_index<request_msg, request_add_interface_source>():
        return (to_request<request_add_interface_source>(*msg));
    case variant_index<request_msg, request_del_interface_source>():
        return (to_request<request_del_interface_source>(*msg));
    case variant_index<request_msg, request_add_port_sink>():
        return (to_request<request_add_port_sink>(*msg));
    case variant_index<request_msg, request_del_port_sink>():
        return (to_request<request_del_port_sink>(*msg));
    case variant_index<request_msg, request_add_port_source>():
        return (to_request<request_add_port_source>(*msg));
    case variant_index<request_msg, request_del_port_source>():
        return (to_request<request_del_port_source>(*msg));
    default: return (std::nullopt);
    }
}

int send_reply(void* socket, const reply_msg reply)
{
    return (send_message(socket, to_serialized_message(std::move(reply))));
}

}
