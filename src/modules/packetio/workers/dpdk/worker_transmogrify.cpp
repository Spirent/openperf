#include <cassert>

#include <zmq.h>

#include "packetio/message_utils.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::packetio::dpdk::worker {

struct serialized_msg {
    zmq_msg_t type;
    zmq_msg_t data;
};

static void close(serialized_msg& msg)
{
    zmq_msg_close(&msg.type);
    zmq_msg_close(&msg.data);
}

static serialized_msg serialize_message(const command_msg& msg)
{
    serialized_msg serialized;
    auto error = (message::zmq_msg_init(&serialized.type, msg.index())
                  || std::visit(utils::overloaded_visitor(
                                    [&](const start_msg& start) {
                                        assert(start.endpoint.length());
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      start.endpoint.data(),
                                                                      start.endpoint.length()));
                                    },
                                    [&](const stop_msg& stop) {
                                        assert(stop.endpoint.length());
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      stop.endpoint.data(),
                                                                      stop.endpoint.length()));
                                    },
                                    [&](const add_descriptors_msg& add) {
                                        assert(add.descriptors.size());
                                        auto& vec = add.descriptors;
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      vec.data(),
                                                                      sizeof(*std::begin(vec)) * vec.size()));
                                    },
                                    [&](const del_descriptors_msg& del) {
                                        assert(del.descriptors.size());
                                        auto& vec = del.descriptors;
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      vec.data(),
                                                                      sizeof(*std::begin(vec)) * vec.size()));
                                    }),
                                msg));

    if (error) {
        throw std::bad_alloc();
    }

    return (serialized);
}

int send_message(void *socket, const command_msg& cmd)
{
    /*
     * Note: sendmsg takes ownership of the messages's buffer, so we only
     * need to clean up on errors.
     */
    auto msg = serialize_message(cmd);
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

static command_msg deserialize_message(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<command_msg>().index());
    auto idx = *(message::zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<command_msg, start_msg>(): {
        std::string data(message::zmq_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (start_msg{ data });
    }
    case utils::variant_index<command_msg, stop_msg>(): {
        std::string data(message::zmq_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (stop_msg{ data });
    }
    case utils::variant_index<command_msg, add_descriptors_msg>(): {
        auto data = message::zmq_msg_data<descriptor*>(&msg.data);
        auto count = zmq_msg_size(&msg.data) / sizeof(descriptor);
        std::vector<descriptor> descriptors(data, data + count);
        return (add_descriptors_msg{ descriptors });
    }
    case utils::variant_index<command_msg, del_descriptors_msg>(): {
        auto data = message::zmq_msg_data<descriptor*>(&msg.data);
        auto count = zmq_msg_size(&msg.data) / sizeof(descriptor);
        std::vector<descriptor> descriptors(data, data + count);
        return (del_descriptors_msg{ descriptors });
    }
    default:
        throw std::runtime_error("Unhandled deserialization case; fix me!");
    }
}

std::optional<command_msg> recv_message(void* socket, int flags)
{
    serialized_msg msg;
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
