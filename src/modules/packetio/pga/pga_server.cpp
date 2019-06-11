#include <zmq.h>

#include "packetio/pga/pga_api.h"
#include "packetio/pga/pga_server.h"

namespace icp::packetio::pga::api {

std::string_view endpoint = "inproc://icp_packetio_pga";

#define STRING_VIEW_TO_C_STR(sv) static_cast<int>(sv.length()), sv.data()

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static reply_msg handle_add_interface_sink(stack::generic_stack& stack,
                                           request_add_interface_sink& msg)
{
    if (auto success = stack.attach_interface_sink(msg.id, msg.item); !success) {
        return (reply_error{
                .code = success.error(),
                .mesg = ("Failed to attach sink " + msg.item.id()
                         + " to interface " + std::string(msg.id)
                         + ": " + std::string(strerror(success.error()))) });
    }

    return (reply_ok{});
}

static reply_msg handle_del_interface_sink(stack::generic_stack& stack,
                                           request_del_interface_sink& msg)
{
    stack.detach_interface_sink(msg.id, msg.item);
    return (reply_ok{});
}

static reply_msg handle_add_interface_source(stack::generic_stack& stack,
                                             request_add_interface_source& msg)
{
    if (auto success = stack.attach_interface_source(msg.id, msg.item); !success) {
        return (reply_error{
                .code = success.error(),
                .mesg = ("Failed to attach source " + msg.item.id()
                         + " to interface " + std::string(msg.id)
                         + ": " + std::string(strerror(success.error()))) });
    }

    return (reply_ok{});
}

static reply_msg handle_del_interface_source(stack::generic_stack& stack,
                                             request_del_interface_source& msg)
{
    stack.detach_interface_source(msg.id, msg.item);
    return (reply_ok{});
}

static reply_msg handle_add_port_sink(driver::generic_driver& driver,
                                      request_add_port_sink& msg)
{
    if (auto success = driver.attach_port_sink(msg.id, msg.item); !success) {
        return (reply_error{
                .code = success.error(),
                .mesg = ("Failed to attach sink " + msg.item.id()
                         + " to port " + std::string(msg.id)
                         + ": " + std::string(strerror(success.error()))) });
    }

    return (reply_ok{});
}

static reply_msg handle_del_port_sink(driver::generic_driver& driver,
                                      request_del_port_sink& msg)
{
    driver.detach_port_sink(msg.id, msg.item);
    return (reply_ok{});
}

static reply_msg handle_add_port_source(driver::generic_driver& driver,
                                        request_add_port_source& msg)
{
    if (auto success = driver.attach_port_source(msg.id, msg.item); !success) {
        return (reply_error{
                .code = success.error(),
                .mesg = ("Failed to attach source " + msg.item.id()
                         + " to port " + std::string(msg.id)
                         + ": " + std::string(strerror(success.error()))) });
    }

    return (reply_ok{});
}

static reply_msg handle_del_port_source(driver::generic_driver& driver,
                                        request_del_port_source& msg)
{
    driver.detach_port_source(msg.id, msg.item);
    return (reply_ok{});
}

static std::string to_string(request_msg& msg)
{
    return (std::visit(overloaded_visitor(
                           [](const request_add_interface_sink& msg) {
                               return ("add sink " + msg.item.id() + " to interface " + msg.id);
                           },
                           [](const request_del_interface_sink& msg) {
                               return ("delete sink " + msg.item.id() + " from interface " + msg.id);
                           },
                           [](const request_add_interface_source& msg) {
                               return ("add source " + msg.item.id() + " to interface " + msg.id);
                           },
                           [](const request_del_interface_source& msg) {
                               return ("delete source " + msg.item.id() + " from interface " + msg.id);
                           },
                           [](const request_add_port_sink& msg) {
                               return ("add sink " + msg.item.id() + " to port " + msg.id);
                           },
                           [](const request_del_port_sink& msg) {
                               return ("delete sink " + msg.item.id() + " from port " + msg.id);
                           },
                           [](const request_add_port_source& msg) {
                               return ("add source " + msg.item.id() + " to port " + msg.id);
                           },
                           [](const request_del_port_source& msg) {
                               return ("delete source " + msg.item.id() + " from port " + msg.id);
                           }),
                       msg));
}

static int handle_rpc_request(const icp_event_data *data, void *arg)
{
    auto server = reinterpret_cast<pga::api::server*>(arg);
    unsigned tx_errors = 0;

    while (auto request = recv_request(data->socket, ZMQ_DONTWAIT)) {
        ICP_LOG(ICP_LOG_TRACE, "Received request to %s\n", to_string(*request).c_str());

        auto reply = std::visit(overloaded_visitor(
                                    [&](request_add_interface_sink& msg) -> reply_msg {
                                        return (handle_add_interface_sink(server->stack(), msg));
                                    },
                                    [&](request_del_interface_sink& msg) -> reply_msg {
                                        return (handle_del_interface_sink(server->stack(), msg));
                                    },
                                    [&](request_add_interface_source& msg) -> reply_msg {
                                        return (handle_add_interface_source(server->stack(), msg));
                                    },
                                    [&](request_del_interface_source& msg) -> reply_msg {
                                        return (handle_del_interface_source(server->stack(), msg));
                                    },
                                    [&](request_add_port_sink& msg) -> reply_msg {
                                        return (handle_add_port_sink(server->driver(), msg));
                                    },
                                    [&](request_del_port_sink& msg) -> reply_msg {
                                        return (handle_del_port_sink(server->driver(), msg));
                                    },
                                    [&](request_add_port_source& msg) -> reply_msg {
                                        return (handle_add_port_source(server->driver(), msg));
                                    },
                                    [&](request_del_port_source& msg) -> reply_msg {
                                        return (handle_del_port_source(server->driver(), msg));
                                    }),
                                *request);

        if (send_reply(data->socket, reply) == -1) {
            tx_errors++;
            ICP_LOG(ICP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }

        std::visit(overloaded_visitor(
                       [&](reply_ok&) {
                           ICP_LOG(ICP_LOG_TRACE, "Request to %s succeeded\n",
                                   to_string(*request).c_str());
                       },
                       [&](reply_error& error) {
                           ICP_LOG(ICP_LOG_TRACE, "Request to %s failed: %s\n",
                                   to_string(*request).c_str(),
                                   std::string(strerror(error.code)).c_str());
                       }),
                   reply);
    }

    return ((tx_errors || errno == ETERM) ? -1 : 0);
}

server::server(void *context, core::event_loop& loop,
               driver::generic_driver& driver,
               stack::generic_stack& stack)
    : m_socket(icp_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_driver(driver)
    , m_stack(stack)
{
    struct icp_event_callbacks callbacks = {
        .on_read = handle_rpc_request
    };
    loop.add(m_socket.get(), &callbacks, this);
}

driver::generic_driver& server::driver() const
{
    return (m_driver);
}

stack::generic_stack& server::stack() const
{
    return (m_stack);
}

}
