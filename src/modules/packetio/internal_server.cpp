#include "packetio/internal_api.hpp"
#include "packetio/internal_server.hpp"
#include "utils/overloaded_visitor.hpp"
#include "message/serialized_message.hpp"

namespace openperf::packetio::internal::api {

std::string_view endpoint = "inproc://op_packetio_internal";

reply_msg handle_request(server& s, const request_interface_add& request)
{
    OP_LOG(OP_LOG_DEBUG, "internal server/add interface: enter\n");
    auto result =
        s.workers().add_interface(request.data.port_id, request.data.interface);
    if (!result) {
        return (reply_error{result.error()});
    } else {
        OP_LOG(OP_LOG_DEBUG, "internal server/add interface: ok\n");
        return (reply_ok{});
    }
}

reply_msg handle_request(server& s, const request_interface& request)
{
    auto maybe_interface = s.workers().interface(request.interface_id);
    if (!maybe_interface) {
        return (reply_error{maybe_interface.error()});
    } else {
        return (reply_interface{.data.interface = *maybe_interface});
    }
}

reply_msg handle_request(server& s, const request_interface_del& request)
{
    s.workers().del_interface(request.data.port_id, request.data.interface);
    return (reply_ok{});
}

reply_msg handle_request(server& s, const request_port_index& request)
{
    auto index = s.driver().port_index(request.port_id);
    if (!index) {
        return (reply_error{EEXIST});
    } else {
        return (reply_port_index{index.value()});
    }
}

reply_msg handle_request(server& s, const request_transmit_function& request)
{
    auto f = s.workers().get_transmit_function(request.port_id);
    return (reply_transmit_function{f});
}

reply_msg handle_request(server& s, const request_sink_add& request)
{
    auto result = s.workers().add_sink(
        request.data.direction, request.data.src_id, request.data.sink);
    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_ok{});
    }
}

reply_msg handle_request(server& s, const request_sink_del& request)
{
    s.workers().del_sink(
        request.data.direction, request.data.src_id, request.data.sink);
    return (reply_ok{});
}

reply_msg handle_request(server& s, const request_source_add& request)
{
    auto result =
        s.workers().add_source(request.data.dst_id, request.data.source);

    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_ok{});
    }
}

reply_msg handle_request(server& s, const request_source_del& request)
{
    s.workers().del_source(request.data.dst_id, request.data.source);
    return (reply_ok{});
}

reply_msg handle_request(server& s, const request_source_swap& request)
{
    auto result = (request.data.action.has_value()
                       ? s.workers().swap_source(request.data.dst_id,
                                                 request.data.outgoing,
                                                 request.data.incoming,
                                                 request.data.action.value())
                       : s.workers().swap_source(request.data.dst_id,
                                                 request.data.outgoing,
                                                 request.data.incoming));

    if (!result) {
        return (reply_error{result.error()});
    } else {
        return (reply_ok{});
    }
}

reply_msg handle_request(server& s, const request_task_add& request)
{
    auto result = (request.data.on_delete.has_value()
                       ? s.workers().add_task(request.data.ctx,
                                              request.data.name,
                                              request.data.notifier,
                                              request.data.on_event,
                                              request.data.on_delete.value(),
                                              request.data.arg)
                       : s.workers().add_task(request.data.ctx,
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

reply_msg handle_request(server& s, const request_task_del& request)
{
    s.workers().del_task(request.task_id);
    return (reply_ok{});
}

reply_msg handle_request(server& s, const request_worker_ids& request)
{
    return (reply_worker_ids{
        s.workers().get_worker_ids(request.direction, request.object_id)});
}

static std::string to_string(request_msg& request)
{
    return (std::visit(
        utils::overloaded_visitor(
            [](const request_interface_add& msg) {
                return ("add interface " + msg.data.interface.id() + " to port "
                        + std::string(msg.data.port_id));
            },
            [](const request_interface& msg) {
                return ("get interface " + msg.interface_id);
            },
            [](const request_interface_del& msg) {
                return ("delete interface " + msg.data.interface.id()
                        + " from port " + std::string(msg.data.port_id));
            },
            [](const request_port_index& msg) {
                return ("get index for port " + msg.port_id);
            },
            [](const request_transmit_function& msg) {
                return ("get transmit function for port " + msg.port_id);
            },
            [](const request_sink_add& msg) {
                return ("add sink " + msg.data.sink.id() + " to source "
                        + std::string(msg.data.src_id));
            },
            [](const request_sink_del& msg) {
                return ("delete sink " + msg.data.sink.id() + " from source "
                        + std::string(msg.data.src_id));
            },
            [](const request_source_add& msg) {
                return ("add source " + std::string(msg.data.source.id())
                        + " to destination " + std::string(msg.data.dst_id));
            },
            [](const request_source_del& msg) {
                return ("delete source " + std::string(msg.data.source.id())
                        + " from destination " + std::string(msg.data.dst_id));
            },
            [](const request_source_swap& msg) {
                return ("swap source " + msg.data.outgoing.id() + " with "
                        + msg.data.incoming.id() + " on destination "
                        + std::string(msg.data.dst_id));
            },
            [](const request_task_add& msg) {
                return ("add task " + std::string(msg.data.name));
            },
            [](const request_task_del& msg) {
                return ("delete task " + std::string(msg.task_id));
            },
            [](const request_worker_ids& msg) {
                std::string direction_str;
                switch (msg.direction) {
                case packet::traffic_direction::NONE:
                    direction_str = "NONE";
                    break;
                case packet::traffic_direction::RX:
                    direction_str = "RX";
                    break;
                case packet::traffic_direction::TX:
                    direction_str = "TX";
                    break;
                case packet::traffic_direction::RXTX:
                    direction_str = "RX and TX";
                    break;
                }
                return (
                    "get worker " + direction_str + " ids for "
                    + (msg.object_id ? *msg.object_id : std::string("ALL")));
            }),
        request));
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto server = reinterpret_cast<internal::api::server*>(arg);
    auto reply_errors = 0;

    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        OP_LOG(OP_LOG_TRACE,
               "Received request to %s\n",
               to_string(*request).c_str());

        auto request_visitor = [&](auto& request_msg) -> reply_msg {
            return (handle_request(*server, request_msg));
        };
        auto reply = std::visit(request_visitor, *request);

        if (auto error = std::get_if<reply_error>(&reply)) {
            OP_LOG(OP_LOG_TRACE,
                   "Request to %s failed: %s\n",
                   to_string(*request).c_str(),
                   strerror(error->value));
        } else {
            OP_LOG(OP_LOG_TRACE,
                   "Request to %s succeeded\n",
                   to_string(*request).c_str());
        }

        if (message::send(data->socket, serialize_reply(std::move(reply)))
            == -1) {
            reply_errors++;
            OP_LOG(
                OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }
    }

    return ((reply_errors || errno == ETERM) ? -1 : 0);
}

server::server(void* context,
               core::event_loop& loop,
               driver::generic_driver& driver,
               workers::generic_workers& workers)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_driver(driver)
    , m_workers(workers)
{
    struct op_event_callbacks callbacks = {.on_read = handle_rpc_request};

    loop.add(m_socket.get(), &callbacks, this);
}

driver::generic_driver& server::driver() const { return (m_driver); }

workers::generic_workers& server::workers() const { return (m_workers); }

} // namespace openperf::packetio::internal::api
