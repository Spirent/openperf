#ifndef _OP_PACKETIO_INTERNAL_API_HPP_
#define _OP_PACKETIO_INTERNAL_API_HPP_

#include <optional>
#include <string>

#include <zmq.h>
#include "tl/expected.hpp"

#include "packetio/generic_event_loop.hpp"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_sink.hpp"
#include "packetio/generic_source.hpp"
#include "packetio/generic_workers.hpp"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::packetio::internal::api {

using serialized_msg = openperf::message::serialized_message;

constexpr size_t name_length_max = 64;

extern std::string_view endpoint;

struct interface_data
{
    char port_id[name_length_max];
    interface::generic_interface interface;
};

struct request_interface_add
{
    interface_data data;
};

struct request_interface
{
    std::string interface_id;
};

struct request_interface_del
{
    interface_data data;
};

struct request_port_index
{
    std::string port_id;
};

struct sink_data
{
    char src_id[name_length_max];
    packet::traffic_direction direction;
    packet::generic_sink sink;
};

struct request_sink_add
{
    sink_data data;
};

struct request_sink_del
{
    sink_data data;
};

struct source_data
{
    char dst_id[name_length_max];
    packet::generic_source source;
};

struct request_source_add
{
    source_data data;
};

struct request_source_del
{
    source_data data;
};

struct source_swap_data
{
    char dst_id[name_length_max];
    packet::generic_source outgoing;
    packet::generic_source incoming;
    std::optional<workers::source_swap_function> action;
};

struct request_source_swap
{
    source_swap_data data;
};

struct task_data
{
    char name[name_length_max];
    workers::context ctx;
    event_loop::event_notifier notifier;
    event_loop::event_handler on_event;
    std::optional<event_loop::delete_handler> on_delete;
    std::any arg;
};

struct request_task_add
{
    task_data data;
};

struct request_task_del
{
    std::string task_id;
};

struct request_worker_ids
{
    std::optional<std::string> object_id = std::nullopt;
    packet::traffic_direction direction;
};

struct reply_interface
{
    interface_data data;
};

struct reply_task_add
{
    std::string task_id;
};

struct request_transmit_function
{
    std::string port_id;
};

struct reply_port_index
{
    int index;
};

struct reply_transmit_function
{
    workers::transmit_function f;
};

struct reply_worker_ids
{
    std::vector<unsigned> worker_ids;
};

struct reply_ok
{};

struct reply_error
{
    int value;
};

using request_msg = std::variant<request_interface,
                                 request_interface_add,
                                 request_interface_del,
                                 request_port_index,
                                 request_sink_add,
                                 request_sink_del,
                                 request_source_add,
                                 request_source_del,
                                 request_source_swap,
                                 request_task_add,
                                 request_task_del,
                                 request_transmit_function,
                                 request_worker_ids>;

using reply_msg = std::variant<reply_interface,
                               reply_port_index,
                               reply_task_add,
                               reply_transmit_function,
                               reply_worker_ids,
                               reply_ok,
                               reply_error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

} // namespace openperf::packetio::internal::api

#endif /* _OP_PACKETIO_INTERNAL_API_HPP_ */
