#ifndef _OP_PACKETIO_INTERNAL_API_HPP_
#define _OP_PACKETIO_INTERNAL_API_HPP_

#include <optional>
#include <string>

#include <zmq.h>
#include "tl/expected.hpp"

#include "packetio/generic_event_loop.hpp"
#include "packetio/generic_sink.hpp"
#include "packetio/generic_source.hpp"
#include "packetio/generic_workers.hpp"

namespace openperf::packetio::internal::api {

constexpr size_t name_length_max = 64;

extern std::string_view endpoint;

struct sink_data
{
    char src_id[name_length_max];
    packets::generic_sink sink;
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
    packets::generic_source source;
};

struct request_source_add
{
    source_data data;
};

struct request_source_del
{
    source_data data;
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

struct request_worker_rx_ids
{
    std::optional<std::string> object_id = std::nullopt;
};

struct request_worker_tx_ids
{
    std::optional<std::string> object_id = std::nullopt;
};

struct reply_task_add
{
    std::string task_id;
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

using request_msg =
    std::variant<request_sink_add, request_sink_del, request_source_add,
                 request_source_del, request_task_add, request_task_del,
                 request_worker_rx_ids, request_worker_tx_ids>;

using reply_msg =
    std::variant<reply_task_add, reply_worker_ids, reply_ok, reply_error>;

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

serialized_msg serialize_request(const request_msg& request);
serialized_msg serialize_reply(const reply_msg& reply);

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg);
tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

} // namespace openperf::packetio::internal::api

#endif /* _OP_PACKETIO_INTERNAL_API_HPP_ */
