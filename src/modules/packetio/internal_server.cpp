#include "packetio/internal_api.h"
#include "packetio/internal_server.h"

namespace icp::packetio::internal::api {

std::string_view endpoint = "inproc://icp_packetio_internal";

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

reply_msg handle_request(workers::generic_workers& workers,
                         const request_sink_add& request)
{
    (void)workers;
    (void)request;
    return (reply_error{ENOSYS});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_sink_del& request)
{
    (void)workers;
    (void)request;
    return (reply_error{ENOSYS});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_source_add& request)
{
    (void)workers;
    (void)request;
    return (reply_error{ENOSYS});
}

reply_msg handle_request(workers::generic_workers& workers,
                         const request_source_del& request)
{
    (void)workers;
    (void)request;
    return (reply_error{ENOSYS});
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

static std::string to_string(request_msg& request)
{
    return (std::visit(overloaded_visitor(
                           [](const request_sink_add& msg) {
                               return ("add sink " + std::string(msg.data.sink.id())
                                       + " to source " + std::string(msg.data.src_id));
                           },
                           [](const request_sink_del& msg) {
                               return ("delete sink " + std::string(msg.sink_id));
                           },
                           [](const request_source_add& msg) {
                               return ("add source " + std::string(msg.data.source.id())
                                       + " to destination " + std::string(msg.data.dst_id));
                           },
                           [](const request_source_del& msg) {
                               return ("delete source " + std::string(msg.source_id));
                           },
                           [](const request_task_add& msg) {
                               return ("add task " + std::string(msg.data.name));
                           },
                           [](const request_task_del& msg) {
                               return ("delete task " + std::string(msg.task_id));
                           }),
                       request));
}

static int handle_rpc_request(const icp_event_data* data, void* arg)
{
    auto server = reinterpret_cast<internal::api::server*>(arg);
    unsigned tx_errors = 0;

    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
           .and_then(deserialize_request)) {
        ICP_LOG(ICP_LOG_TRACE, "Received request to %s\n", to_string(*request).c_str());

        auto handle_visitor = [&](auto& request_msg) -> reply_msg {
                                  return (handle_request(server->workers(), request_msg));
                              };
        auto reply = std::visit(handle_visitor, *request);

        if (send_message(data->socket, serialize_reply(reply)) == -1) {
            tx_errors++;
            ICP_LOG(ICP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }

        if (auto error = std::get_if<reply_error>(&reply)) {
            ICP_LOG(ICP_LOG_TRACE, "Request to %s failed: %s\n",
                    to_string(*request).c_str(), strerror(error->value));
        } else {
            ICP_LOG(ICP_LOG_TRACE, "Request to %s succeeded\n",
                    to_string(*request).c_str());
        }
    }

    return ((tx_errors || errno == ETERM) ? -1 : 0);
}

server::server(void* context, core::event_loop& loop,
               workers::generic_workers& workers)
    : m_socket(icp_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_workers(workers)
{
    struct icp_event_callbacks callbacks = {
        .on_read = handle_rpc_request
    };

    loop.add(m_socket.get(), &callbacks, this);
}

workers::generic_workers& server::workers() const
{
    return (m_workers);
}

}
