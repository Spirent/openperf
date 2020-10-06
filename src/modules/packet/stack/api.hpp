#ifndef _OP_PACKET_STACK_API_HPP_
#define _OP_PACKET_STACK_API_HPP_

#include <map>
#include <memory>
#include <string>
#include <variant>

#include "json.hpp"
#include "tl/expected.hpp"

#include "packet/type/net_types.hpp"

namespace swagger::v1::model {
class Interface;
class Stack;
} // namespace swagger::v1::model

namespace openperf::message {
struct serialized_message;
}

namespace openperf::packet::stack::api {

inline constexpr std::string_view endpoint = "inproc://op_packet_stack";

using interface_type = swagger::v1::model::Interface;
using interface_ptr = std::unique_ptr<interface_type>;

using stack_type = swagger::v1::model::Stack;
using stack_ptr = std::unique_ptr<stack_type>;

enum class intf_filter_type {
    none = 0,
    port_id,
    eth_mac_address,
    ipv4_address,
    ipv6_address
};

using intf_filter_value_type = std::variant<std::string,
                                            libpacket::type::mac_address,
                                            libpacket::type::ipv4_address,
                                            libpacket::type::ipv6_address>;

using intf_filter_map_type = std::map<intf_filter_type, intf_filter_value_type>;
using intf_filter_map_ptr = std::unique_ptr<intf_filter_map_type>;

using serialized_msg = openperf::message::serialized_message;

struct request_list_interfaces
{
    intf_filter_map_ptr filter;
};

struct request_list_stacks
{};

struct request_create_interface
{
    interface_ptr interface;
};

struct request_get_interface
{
    std::string id;
};

struct request_get_stack
{
    std::string id;
};

struct request_delete_interface
{
    std::string id;
};

struct request_bulk_create_interfaces
{
    std::vector<interface_ptr> interfaces;
};

struct request_bulk_delete_interfaces
{
    std::vector<std::string> ids;
};

using request_msg = std::variant<request_list_interfaces,
                                 request_create_interface,
                                 request_get_interface,
                                 request_delete_interface,
                                 request_bulk_create_interfaces,
                                 request_bulk_delete_interfaces,
                                 request_list_stacks,
                                 request_get_stack>;

struct reply_ok
{};

enum class error_type { NONE = 0, NOT_FOUND, VERBOSE };

struct reply_error
{
    error_type type = error_type::NONE;
    std::string msg;
};

struct reply_interfaces
{
    std::vector<std::unique_ptr<interface_type>> interfaces;
};

struct reply_stacks
{
    std::vector<std::unique_ptr<stack_type>> stacks;
};

using reply_msg =
    std::variant<reply_ok, reply_error, reply_interfaces, reply_stacks>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

std::string to_string(intf_filter_type);

} // namespace openperf::packet::stack::api

#endif /* _OP_PACKET_STACK_API_HPP_ */
