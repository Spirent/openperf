#include <cassert>

#include <zmq.h>

#include "packetio/workers/dpdk/worker_api.h"

namespace icp::packetio::dpdk::worker {

struct serialized_message {
    zmq_msg_t type;
    zmq_msg_t data;
};

static void close(serialized_message& msg)
{
    zmq_msg_close(&msg.type);
    zmq_msg_close(&msg.data);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
    return (0);
}

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static serialized_message serialize_message(const message& msg)
{
    serialized_message serialized;
    auto error = (zmq_msg_init(&serialized.type, msg.index())
                  || std::visit(overloaded_visitor(
                                    [&](const start_msg& start) {
                                        assert(start.endpoint.length());
                                        return (zmq_msg_init(&serialized.data,
                                                             start.endpoint.data(),
                                                             start.endpoint.length()));
                                    },
                                    [&](const stop_msg& stop) {
                                        assert(stop.endpoint.length());
                                        return (zmq_msg_init(&serialized.data,
                                                             stop.endpoint.data(),
                                                             stop.endpoint.length()));
                                    },
                                    [&](const configure_msg& config) {
                                        assert(config.descriptors.size());
                                        auto& vec = config.descriptors;
                                        return (zmq_msg_init(&serialized.data,
                                                             vec.data(),
                                                             sizeof(*std::begin(vec)) * vec.size()));
                                    }),
                                msg));

    if (error) {
        throw std::bad_alloc();
    }

    return (serialized);
}

int send_message(void *socket, const message& original)
{
    /*
     * Note: sendmsg takes ownership of the messages's buffer, so we only
     * need to clean up on errors.
     */
    auto msg = serialize_message(original);
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

/**
 * Templated function to retrieve the index for a specific type in a variant.
 * Helps make the switch statement in the deserialization function a little
 * more intuitive.
 */
template <typename VariantType, typename T, std::size_t index = 0>
constexpr size_t variant_index() {
    if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) {
        return (index);
    } else {
        return (variant_index<VariantType, T, index + 1>());
    }
}

template <typename T>
static T get_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

static message deserialize_message(const serialized_message& msg)
{
    using index_type = decltype(std::declval<message>().index());
    auto idx = *(get_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case variant_index<message, start_msg>(): {
        std::string data(get_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (start_msg{ .endpoint = data });
    }
    case variant_index<message, stop_msg>(): {
        std::string data(get_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (stop_msg{ .endpoint = data });
    }
    case variant_index<message, configure_msg>(): {
        auto data = get_msg_data<descriptor*>(&msg.data);
        auto count = zmq_msg_size(&msg.data) / sizeof(descriptor);
        std::vector<descriptor> descriptors(data, data + count);
        return (configure_msg{ .descriptors = descriptors });
    }
    default:
        throw std::runtime_error("Unhandled deserialization case; fix me!");
    }
}

std::optional<message> recv_message(void* socket, int flags)
{
    serialized_message msg;
    if (zmq_msg_init(&msg.type) == -1
        || zmq_msg_init(&msg.data) == -1) {
        close(msg);
        return (std::nullopt);
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);
        return (std::nullopt);
    }

    return (std::make_optional(deserialize_message(msg)));
}

}
