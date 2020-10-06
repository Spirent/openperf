#include <zmq.h>

#include "message/serialized_message.hpp"
#include "packet/stack/server.hpp"
#include "utils/overloaded_visitor.hpp"

#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Stack.h"

namespace openperf::packet::stack::api {

static std::string to_string(const request_msg& request)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const request_list_interfaces&) {
                               return (std::string("list interfaces"));
                           },
                           [](const request_create_interface&) {
                               return (std::string("create interface"));
                           },
                           [](const request_get_interface& request) {
                               return ("get interface " + request.id);
                           },
                           [](const request_delete_interface& request) {
                               return ("delete interface " + request.id);
                           },
                           [](const request_bulk_create_interfaces&) {
                               return (std::string("bulk create interfaces"));
                           },
                           [](const request_bulk_delete_interfaces&) {
                               return (std::string("bulk delete interfaces"));
                           },
                           [](const request_list_stacks&) {
                               return (std::string("list stacks"));
                           },
                           [](const request_get_stack& request) {
                               return ("get stack " + request.id);
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

int handle_rpc_request(const op_event_data* data, void* arg)
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

server::server(void* context, core::event_loop& loop, generic_stack& stack)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
    , m_loop(loop)
    , m_stack(stack)
{
    auto callbacks = op_event_callbacks{.on_read = handle_rpc_request};
    m_loop.add(m_socket.get(), &callbacks, this);
}

bool filter_interface(const packetio::interface::generic_interface& intf,
                      const intf_filter_map_type& m)
{
    using namespace libpacket::type;

    /*
     * Note: the filter map contains a variant with typed data. Use std::get
     * to get the appropriate type. Additionally, use the interface string
     * to convert the text address information to an actual binary type. This
     * ensures correct comparisons regardless of octet or quad separators.
     */
    if (m.count(intf_filter_type::port_id)
        && (std::get<std::string>(m.at(intf_filter_type::port_id))
            == intf.port_id())) {
        return (false);
    }

    if (m.count(intf_filter_type::eth_mac_address)
        && (std::get<mac_address>(m.at(intf_filter_type::eth_mac_address))
            == mac_address(intf.mac_address()))) {
        return (false);
    }

    if (intf.ipv4_address() && m.count(intf_filter_type::ipv4_address)
        && (std::get<ipv4_address>(m.at(intf_filter_type::ipv4_address))
            == ipv4_address(intf.ipv4_address().value()))) {
        return (false);
    }

    if (m.count(intf_filter_type::ipv6_address)) {
        const auto is_match = [&](auto&& maybe_v6) {
            return (
                maybe_v6
                && std::get<ipv6_address>(m.at(intf_filter_type::ipv6_address))
                       == ipv6_address(maybe_v6.value()));
        };

        if (is_match(intf.ipv6_address())
            || is_match(intf.ipv6_linklocal_address())) {
            return (false);
        }
    }

    return (true);
}

reply_msg server::handle_request(const request_list_interfaces& request)
{
    /*
     * Retrieve the list of interface ids and use that to generate
     * a vector of interface objects
     */
    auto ids = m_stack.interface_ids();
    using generic_interface = packetio::interface::generic_interface;
    auto interfaces = std::vector<generic_interface>{};
    std::transform(
        std::begin(ids),
        std::end(ids),
        std::back_inserter(interfaces),
        [&](const auto id) { return (m_stack.interface(id).value()); });

    /* Filter out any interfaces if necessary */
    if (request.filter) {
        interfaces.erase(
            std::remove_if(std::begin(interfaces),
                           std::end(interfaces),
                           [&](const auto& intf) {
                               return (filter_interface(intf, *request.filter));
                           }),
            std::end(interfaces));
    }

    /* Turn generic port objects into swagger objects */
    auto reply = reply_interfaces{};
    std::transform(std::begin(interfaces),
                   std::end(interfaces),
                   std::back_inserter(reply.interfaces),
                   [](const auto& intf) {
                       return (
                           packetio::interface::make_swagger_interface(intf));
                   });

    return (reply);
}

reply_msg server::handle_request(const request_create_interface& request)
{
    /*
     * Interfaces must have an ID of some sort
     * so create one if the user didn't.
     */
    if (request.interface->getId().empty()) {
        request.interface->setId(core::to_string(core::uuid::random()));
    }

    const auto id = request.interface->getId();
    auto result = m_stack.create_interface(
        packetio::interface::make_config_data(*request.interface));
    if (!result) {
        return (
            reply_error{.type = error_type::VERBOSE, .msg = result.error()});
    }

    auto reply = reply_interfaces{};
    reply.interfaces.emplace_back(
        make_swagger_interface(m_stack.interface(id).value()));
    return (reply);
}

reply_msg server::handle_request(const request_get_interface& request)
{
    auto intf = m_stack.interface(request.id);
    if (!intf) { return (reply_error{.type = error_type::NOT_FOUND}); }

    auto reply = reply_interfaces{};
    reply.interfaces.emplace_back(
        packetio::interface::make_swagger_interface(*intf));
    return (reply);
}

reply_msg server::handle_request(const request_delete_interface& request)
{
    m_stack.delete_interface(request.id);
    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_create_interfaces& request)
{
    auto bulk_reply = reply_interfaces{};
    auto bulk_errors = std::vector<reply_error>{};

    /* XXX: making a copy due to a bad choice in function signature */
    std::for_each(
        std::begin(request.interfaces),
        std::end(request.interfaces),
        [&](const auto& intf) {
            auto api_reply = handle_request(request_create_interface{
                std::make_unique<interface_type>(*intf)});
            if (auto reply = std::get_if<reply_interfaces>(&api_reply)) {
                assert(reply->interfaces.size() == 1);
                std::move(std::begin(reply->interfaces),
                          std::end(reply->interfaces),
                          std::back_inserter(bulk_reply.interfaces));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Roll back */
        std::for_each(std::begin(bulk_reply.interfaces),
                      std::end(bulk_reply.interfaces),
                      [&](const auto& intf) {
                          handle_request(
                              request_delete_interface{intf->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_delete_interfaces& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_delete_interface{id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_list_stacks&)
{
    auto reply = reply_stacks{};
    reply.stacks.emplace_back(make_swagger_stack(m_stack));
    return (reply);
}

reply_msg server::handle_request(const request_get_stack& request)
{
    if (request.id != m_stack.id()) {
        return (reply_error{.type = error_type::NOT_FOUND});
    }

    auto reply = reply_stacks{};
    reply.stacks.emplace_back(make_swagger_stack(m_stack));
    return (reply);
}

} // namespace openperf::packet::stack::api
