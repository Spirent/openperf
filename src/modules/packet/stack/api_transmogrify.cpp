#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/stack/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Stack.h"

namespace openperf::packet::stack::api {

serialized_msg serialize_request(request_msg&& msg)
{
    auto serialized = serialized_msg{};
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_interfaces& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](request_create_interface& request) {
                     return (message::push(serialized,
                                           std::move(request.interface)));
                 },
                 [&](const request_get_interface& request) {
                     return (message::push(serialized, request.id));
                 },
                 [&](const request_delete_interface& request) {
                     return (message::push(serialized, request.id));
                 },
                 [&](request_bulk_create_interfaces& request) {
                     return (message::push(serialized, request.interfaces));
                 },
                 [&](const request_bulk_delete_interfaces& request) -> int {
                     auto failures = false;
                     std::for_each(std::begin(request.ids),
                                   std::end(request.ids),
                                   [&](const auto& id) {
                                       failures |=
                                           message::push(serialized, id);
                                   });
                     return (failures);
                 },
                 [](const request_list_stacks&) { return (0); },
                 [&](const request_get_stack& request) {
                     return (message::push(serialized, request.id));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    auto serialized = serialized_msg{};
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](reply_interfaces& reply) {
                               return (
                                   message::push(serialized, reply.interfaces));
                           },
                           [&](reply_stacks& reply) {
                               return (message::push(serialized, reply.stacks));
                           },
                           [&](const reply_ok&) { return (0); },
                           [&](const reply_error& error) -> int {
                               return (message::push(serialized, error.type)
                                       || message::push(serialized, error.msg));
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request_list_interfaces>(): {
        auto request = request_list_interfaces{};
        request.filter.reset(message::pop<intf_filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_interface>(): {
        auto request = request_create_interface{};
        request.interface.reset(message::pop<interface_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_interface>(): {
        return (request_get_interface{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_interface>(): {
        return (request_delete_interface{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_create_interfaces>(): {
        return (request_bulk_create_interfaces{
            message::pop_unique_vector<interface_type>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_delete_interfaces>(): {
        auto request = request_bulk_delete_interfaces{};
        auto s = message::pop_string(msg);
        while (!s.empty()) {
            request.ids.emplace_back(s);
            s = message::pop_string(msg);
        }
        return (request);
    }
    case utils::variant_index<request_msg, request_list_stacks>(): {
        return (request_list_stacks{});
    }
    case utils::variant_index<request_msg, request_get_stack>(): {
        return (request_get_stack{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_interfaces>(): {
        return (
            reply_interfaces{message::pop_unique_vector<interface_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_stacks>(): {
        return (reply_stacks{message::pop_unique_vector<stack_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{.type = message::pop<error_type>(msg),
                            .msg = message::pop_string(msg)});
    }

    return (tl::make_unexpected(EINVAL));
}

} // namespace openperf::packet::stack::api
