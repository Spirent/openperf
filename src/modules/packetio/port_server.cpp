#include "zmq.h"

#include "packetio/port_server.hpp"
#include "message/serialized_message.hpp"
#include "utils/overloaded_visitor.hpp"

#include "swagger/v1/model/Port.h"

namespace openperf::packetio::port::api {

using generic_driver = openperf::packetio::driver::generic_driver;

static std::string to_string(const request_msg& request)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const request_list_ports&) {
                               return (std::string("list ports"));
                           },
                           [](const request_create_port&) {
                               return (std::string("create port"));
                           },
                           [](const request_get_port& request) {
                               return ("get port " + request.id);
                           },
                           [](const request_update_port&) {
                               return (std::string("update port"));
                           },
                           [](const request_delete_port& request) {
                               return ("delete port " + request.id);
                           }),
                       request));
}

static std::string to_string(const reply_msg& reply)
{
    if (auto error = std::get_if<reply_error>(&reply)) {
        return ("failed: " + error->msg);
    }

    return ("succeeded");
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {

        OP_LOG(OP_LOG_TRACE,
               "Received request to %s\n",
               to_string(*request).c_str());

        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

        OP_LOG(OP_LOG_TRACE,
               "Request to %s %s\n",
               to_string(*request).c_str(),
               to_string(reply).c_str());

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
               openperf::core::event_loop& loop,
               generic_driver& driver)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_driver(driver)
{
    struct op_event_callbacks callbacks = {.on_read = handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

reply_msg server::handle_request(const request_list_ports& request)
{
    /*
     * Retrieve the list of port ids and use that to generate
     * a vector of port objects.
     */
    auto ids = m_driver.port_ids();
    auto ports = std::vector<port::generic_port>{};
    std::transform(std::begin(ids),
                   std::end(ids),
                   std::back_inserter(ports),
                   [&](const auto id) { return (m_driver.port(id).value()); });

    /* Filter out any ports if necessary */
    if (request.filter && request.filter->count(filter_key_type::kind)) {
        auto& kind = (*request.filter)[filter_key_type::kind];
        ports.erase(std::remove_if(std::begin(ports),
                                   std::end(ports),
                                   [&](const auto& port) {
                                       return (port.kind() != kind);
                                   }),
                    std::end(ports));
    }

    /* Turn generic port objects into swagger objects */
    auto reply = reply_ports{};
    std::transform(std::begin(ports),
                   std::end(ports),
                   std::back_inserter(reply.ports),
                   [](const auto& port) { return (make_swagger_port(port)); });

    return (reply);
};

reply_msg server::handle_request(const request_create_port& request)
{
    const auto id = request.port->getId();
    auto result = m_driver.create_port(id, make_config_data(*request.port));
    if (!result) {
        return (
            reply_error{.type = error_type::VERBOSE, .msg = result.error()});
    }

    auto reply = reply_ports{};
    reply.ports.emplace_back(make_swagger_port(m_driver.port(id).value()));
    return (reply);
};

reply_msg server::handle_request(const request_get_port& request)
{
    auto port = m_driver.port(request.id);
    if (!port) { return (reply_error{.type = error_type::NOT_FOUND}); }

    auto reply = reply_ports{};
    reply.ports.emplace_back(make_swagger_port(*port));
    return (reply);
};

reply_msg server::handle_request(const request_update_port& request)
{
    auto port = m_driver.port(request.port->getId());
    if (!port) { return (reply_error{.type = error_type::NOT_FOUND}); }

    auto success = port->config(make_config_data(*request.port));
    if (!success) {
        return (
            reply_error{.type = error_type::VERBOSE, .msg = success.error()});
    }

    auto reply = reply_ports{};
    reply.ports.emplace_back(
        make_swagger_port(m_driver.port(request.port->getId()).value()));
    return (reply);
};

reply_msg server::handle_request(const request_delete_port& request)
{
    auto success = m_driver.delete_port(request.id);
    if (!success) {
        return (
            reply_error{.type = error_type::VERBOSE, .msg = success.error()});
    }

    return (reply_ok{});
};

} // namespace openperf::packetio::port::api
