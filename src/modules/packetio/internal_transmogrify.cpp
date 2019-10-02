#include <zmq.h>

#include "packetio/message_utils.h"
#include "packetio/internal_api.h"

namespace icp::packetio::internal::api {

void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

serialized_msg serialize_request(const request_msg& msg)
{
    serialized_msg serialized;
    auto error = (message::zmq_msg_init(&serialized.type, msg.index())
                  || std::visit(message::overloaded_visitor(
                                    [&](const request_sink_add& sink_add) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      std::addressof(sink_add.data),
                                                                      sizeof(sink_add.data)));
                                    },
                                    [&](const request_sink_del& sink_del) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      std::addressof(sink_del.data),
                                                                      sizeof(sink_del.data)));
                                    },
                                    [&](const request_source_add& source_add) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      std::addressof(source_add.data),
                                                                      sizeof(source_add.data)));
                                    },
                                    [&](const request_source_del& source_del) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      std::addressof(source_del.data),
                                                                      sizeof(source_del.data)));
                                    },
                                    [&](const request_task_add& task_add) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      std::addressof(task_add.data),
                                                                      sizeof(task_add.data)));
                                    },
                                    [&](const request_task_del& task_del) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      task_del.task_id.data(),
                                                                      task_del.task_id.length()));
                                    },
                                    [&](const request_worker_rx_count&) {
                                        return (message::zmq_msg_init(&serialized.data, 0));
                                    },
                                    [&](const request_worker_tx_count&) {
                                        return (message::zmq_msg_init(&serialized.data, 0));
                                    }),
                                msg));
    if (error) {
        throw std::bad_alloc();
    }

    return (serialized);
}

serialized_msg serialize_reply(const reply_msg& msg)
{
    serialized_msg serialized;
    auto error = (message::zmq_msg_init(&serialized.type, msg.index())
                  || std::visit(message::overloaded_visitor(
                                    [&](const reply_task_add& task_add) {
                                        return (message::zmq_msg_init(&serialized.data,
                                                                      task_add.task_id.data(),
                                                                      task_add.task_id.length()));
                                    },
                                    [&](const reply_ok&) {
                                        return (message::zmq_msg_init(&serialized.data, 0));
                                    },
                                    [&](const reply_count& count) {
                                        return (message::zmq_msg_init(&serialized.data, count.value));
                                    },
                                    [&](const reply_error& error) {
                                        return (message::zmq_msg_init(&serialized.data, error.value));
                                    }),
                                msg));
    if (error) {
        throw std::bad_alloc();
    }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(message::zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case message::variant_index<request_msg, request_sink_add>():
        return (request_sink_add{ *(message::zmq_msg_data<sink_data*>(&msg.data)) });
    case message::variant_index<request_msg, request_sink_del>():
        return (request_sink_del{ *(message::zmq_msg_data<sink_data*>(&msg.data)) });
    case message::variant_index<request_msg, request_source_add>():
        return (request_source_add{ *(message::zmq_msg_data<source_data*>(&msg.data)) });
    case message::variant_index<request_msg, request_source_del>():
        return (request_source_del{ *(message::zmq_msg_data<source_data*>(&msg.data)) });
    case message::variant_index<request_msg, request_task_add>():
        return (request_task_add{ *(message::zmq_msg_data<task_data*>(&msg.data)) });
    case message::variant_index<request_msg, request_task_del>(): {
        std::string data(message::zmq_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (request_task_del{ data });
    }
    case message::variant_index<request_msg, request_worker_rx_count>():
        return (request_worker_rx_count{});
    case message::variant_index<request_msg, request_worker_tx_count>():
        return (request_worker_tx_count{});
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(message::zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case message::variant_index<reply_msg, reply_task_add>(): {
        std::string data(message::zmq_msg_data<char*>(&msg.data),
                         zmq_msg_size(&msg.data));
        return (reply_task_add{ data });
    }
    case message::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case message::variant_index<reply_msg, reply_count>():
        return (reply_count{ *(message::zmq_msg_data<unsigned*>(&msg.data)) });
    case message::variant_index<reply_msg, reply_error>():
        return (reply_error{ *(message::zmq_msg_data<int*>(&msg.data)) });
    }

    return (tl::make_unexpected(EINVAL));
}

int send_message(void* socket, serialized_msg&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1
        || zmq_msg_init(&msg.data) == -1) {
        close(msg);
        return (tl::make_unexpected(ENOMEM));
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);
        return (tl::make_unexpected(errno));
    }

    return (msg);
}

}
