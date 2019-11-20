#include "packetio/internal_api.hpp"
#include "packetio/internal_server.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::internal::api {

std::string_view endpoint = "inproc://op_packetio_internal";

reply_msg handle_request(workers::generic_workers& workers,
                         const request_sink_add& request)
{
    auto result = workers.add_sink(request.data.src_id,
                                   std::move(request.data.sink));
    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_ok{});
    }
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_sink_del& request)
{
    workers.del_sink(request.data.src_id,
                     std::move(request.data.sink));
    return (reply_ok{});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_source_add& request)
{
    auto result = workers.add_source(request.data.dst_id,
                                     std::move(request.data.source));

    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_ok{});
    }
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_source_del& request)
{
    workers.del_source(request.data.dst_id,
                       std::move(request.data.source));
    return (reply_ok{});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_task_add& request)
{
    auto result = (request.data.on_delete.has_value()
                   ? workers.add_task(request.data.ctx,
                                      request.data.name,
                                      request.data.notifier,
                                      request.data.on_event,
                                      request.data.on_delete.value(),
                                      request.data.arg)
                   : workers.add_task(request.data.ctx,
                                      request.data.name,
                                      request.data.notifier,
                                      request.data.on_event,
                                      request.data.arg));

    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_task_add{std::move(*result)});
    }
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_task_del& request)
{
    workers.del_task(request.task_id);
    return (reply_ok{});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_worker_rx_ids& rx_ids)
{
    return (reply_worker_ids{workers.get_rx_worker_ids(rx_ids.object_id)});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_worker_tx_ids& tx_ids)
{
    return (reply_worker_ids{workers.get_tx_worker_ids(tx_ids.object_id)});
}

static std::string to_string(request_msg& request)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const request_sink_add& msg) {
                               return ("add sink " + std::string(msg.data.sink.id())
                                       + " to source " + std::string(msg.data.src_id));
                           },
                           [](const request_sink_del& msg) {
                               return ("delete sink " + std::string(msg.data.sink.id())
                                       + " from source " + std::string(msg.data.src_id));
                           },
                           [](const request_source_add& msg) {
                               return ("add source " + std::string(msg.data.source.id())
                                       + " to destination " + std::string(msg.data.dst_id));
                           },
                           [](const request_source_del& msg) {
                               return ("delete source " + std::string(msg.data.source.id())
                                       + " from destination " + std::string(msg.data.dst_id));
                           },
                           [](const request_task_add& msg) {
                               return ("add task " + std::string(msg.data.name));
                           },
                           [](const request_task_del& msg) {
                               return ("delete task " + std::string(msg.task_id));
                           },
                           [](const request_worker_rx_ids& rx_ids) {
                               return ("get worker RX ids for "
                                       + (rx_ids.object_id ? *rx_ids.object_id : std::string("ALL")));
                           },
                           [](const request_worker_tx_ids& tx_ids) {
                               return ("get worker TX ids for "
                                       + (tx_ids.object_id ? *tx_ids.object_id : std::string("ALL")));
                           }),
                       request));
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto server = reinterpret_cast<internal::api::server*>(arg);
    unsigned tx_errors = 0;

    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
           .and_then(deserialize_request)) {
        OP_LOG(OP_LOG_TRACE, "Received request to %s\n", to_string(*request).c_str());

        auto handle_visitor = [&](auto& request_msg) -> reply_msg {
                                  return (handle_request(server->workers(), request_msg));
                              };
        auto reply = std::visit(handle_visitor, *request);

        if (send_message(data->socket, serialize_reply(reply)) == -1) {
            tx_errors++;
            OP_LOG(OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }

        if (auto error = std::get_if<reply_error>(&reply)) {
            OP_LOG(OP_LOG_TRACE, "Request to %s failed: %s\n",
                    to_string(*request).c_str(), strerror(error->value));
        } else {
            OP_LOG(OP_LOG_TRACE, "Request to %s succeeded\n",
                    to_string(*request).c_str());
        }
    }

    return ((tx_errors || errno == ETERM) ? -1 : 0);
}

server::server(void* context, core::event_loop& loop,
               workers::generic_workers& workers)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_workers(workers)
{
    struct op_event_callbacks callbacks = {
        .on_read = handle_rpc_request
    };

    loop.add(m_socket.get(), &callbacks, this);
}

workers::generic_workers& server::workers() const
{
    return (m_workers);
}

}
