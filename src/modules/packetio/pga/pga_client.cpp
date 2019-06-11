#include <algorithm>
#include <iostream>

#include <tl/expected.hpp>
#include <zmq.h>

#include "packetio/pga/pga_api.h"
#include "packetio/pga/pga_client.h"

namespace icp::packetio::pga::api {

client::client(void* context)
    : m_socket(icp_socket_get_client(context, ZMQ_REQ, api::endpoint.data()))
{}

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static tl::expected<void, reply_error> to_client_return_type(const reply_msg&& reply)
{
    return (std::visit(overloaded_visitor(
                           [](const reply_ok&) -> tl::expected<void, reply_error> {
                               return {};
                           },
                           [](const reply_error& error) -> tl::expected<void, reply_error> {
                               return (tl::make_unexpected(error));
                           }), std::move(reply)));
}

tl::expected<void,reply_error> client::interface_add(std::string_view id, generic_sink& sink)
{
    auto request = request_add_interface_sink{ .id = std::string(id),
                                               .item = sink };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::interface_del(std::string_view id, generic_sink& sink)
{
    auto request = request_del_interface_sink{ .id = std::string(id),
                                               .item = sink };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::interface_add(std::string_view id, generic_source& source)
{
    auto request = request_add_interface_source{ .id = std::string(id),
                                                 .item = source };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::interface_del(std::string_view id, generic_source& source)
{
    auto request = request_del_interface_source{ .id = std::string(id),
                                                 .item = source };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::port_add(std::string_view id, generic_sink& sink)
{
    auto request = request_add_port_sink{ .id = std::string(id),
                                          .item = sink };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::port_del(std::string_view id, generic_sink& sink)
{
    auto request = request_del_port_sink{ .id = std::string(id),
                                          .item = sink };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::port_add(std::string_view id, generic_source& source)
{
    auto request = request_add_port_source{ .id = std::string(id),
                                            .item = source };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

tl::expected<void,reply_error> client::port_del(std::string_view id, generic_source& source)
{
    auto request = request_del_port_source{ .id = std::string(id),
                                            .item = source };

    return (to_client_return_type(submit_request(m_socket.get(), request)));
}

}
