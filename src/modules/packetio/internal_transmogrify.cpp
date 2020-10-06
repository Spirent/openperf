#include <zmq.h>

#include "message/serialized_message.hpp"
#include "packetio/internal_api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::packetio::internal::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_interface_add& intf_add) {
                     return (message::push(serialized,
                                           std::addressof(intf_add.data),
                                           sizeof(intf_add.data)));
                 },
                 [&](const request_interface_del& intf_del) {
                     return (message::push(serialized,
                                           std::addressof(intf_del.data),
                                           sizeof(intf_del.data)));
                 },
                 [&](const request_port_index& port_index) {
                     return (message::push(serialized, port_index.port_id));
                 },
                 [&](const request_sink_add& sink_add) {
                     return (message::push(serialized,
                                           std::addressof(sink_add.data),
                                           sizeof(sink_add.data)));
                 },
                 [&](const request_sink_del& sink_del) {
                     return (message::push(serialized,
                                           std::addressof(sink_del.data),
                                           sizeof(sink_del.data)));
                 },
                 [&](const request_source_add& source_add) {
                     return (message::push(serialized,
                                           std::addressof(source_add.data),
                                           sizeof(source_add.data)));
                 },
                 [&](const request_source_del& source_del) {
                     return (message::push(serialized,
                                           std::addressof(source_del.data),
                                           sizeof(source_del.data)));
                 },
                 [&](const request_source_swap& source_swap) {
                     return (message::push(serialized,
                                           std::addressof(source_swap.data),
                                           sizeof(source_swap.data)));
                 },
                 [&](const request_task_add& task_add) {
                     return (message::push(serialized,
                                           std::addressof(task_add.data),
                                           sizeof(task_add.data)));
                 },
                 [&](const request_task_del& task_del) {
                     return (message::push(serialized, task_del.task_id));
                 },
                 [&](const request_transmit_function& tx_function) {
                     return (message::push(serialized, tx_function.port_id));
                 },
                 [&](const request_worker_ids& worker_ids) -> int {
                     return message::push(serialized, worker_ids.direction)
                            || (worker_ids.object_id ? message::push(
                                    serialized, worker_ids.object_id.value())
                                                     : message::push(serialized,
                                                                     0));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const reply_port_index& port_index) {
                     return (message::push(serialized, port_index.index));
                 },
                 [&](const reply_task_add& task_add) {
                     return (message::push(serialized, task_add.task_id));
                 },
                 [&](const reply_transmit_function& tx_function) {
                     return (message::push(serialized, tx_function.f));
                 },
                 [&](const reply_worker_ids& worker_ids) {
                     return (message::push(serialized, worker_ids.worker_ids));
                 },
                 [&](const reply_ok&) { return (0); },
                 [&](const reply_error& error) {
                     return (message::push(serialized, error.value));
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
    case utils::variant_index<request_msg, request_interface_add>(): {
        auto request = request_interface_add{
            *message::front_msg_data<interface_data*>(msg)};
        message::pop_front(msg);
        return (request);
    }
    case utils::variant_index<request_msg, request_interface_del>(): {
        auto request = request_interface_del{
            *message::front_msg_data<interface_data*>(msg)};
        message::pop_front(msg);
        return (request);
    }
    case utils::variant_index<request_msg, request_port_index>():
        return (request_port_index{message::pop_string(msg)});
    case utils::variant_index<request_msg, request_sink_add>(): {
        auto request =
            request_sink_add{*message::front_msg_data<sink_data*>(msg)};
        message::pop_front(msg);
        return request;
    }
    case utils::variant_index<request_msg, request_sink_del>(): {
        auto request =
            request_sink_del{*message::front_msg_data<sink_data*>(msg)};
        message::pop_front(msg);
        return request;
    }
    case utils::variant_index<request_msg, request_source_add>(): {
        auto request =
            request_source_add{*message::front_msg_data<source_data*>(msg)};
        message::pop_front(msg);
        return request;
    }
    case utils::variant_index<request_msg, request_source_del>(): {
        auto request =
            request_source_del{*message::front_msg_data<source_data*>(msg)};
        message::pop_front(msg);
        return request;
    }
    case utils::variant_index<request_msg, request_source_swap>(): {
        auto request = request_source_swap{
            *message::front_msg_data<source_swap_data*>(msg)};
        message::pop_front(msg);
        return request;
    }
    case utils::variant_index<request_msg, request_task_add>(): {
        auto request = request_task_add{message::pop<task_data>(msg)};
        return request;
    }
    case utils::variant_index<request_msg, request_task_del>():
        return (request_task_del{message::pop_string(msg)});
    case utils::variant_index<request_msg, request_transmit_function>():
        return (request_transmit_function{message::pop_string(msg)});
    case utils::variant_index<request_msg, request_worker_ids>(): {
        auto request = request_worker_ids{};
        request.direction = message::pop<packet::traffic_direction>(msg);
        auto object_id = message::pop_string(msg);
        if (!object_id.empty()) request.object_id = object_id;
        return request;
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_port_index>():
        return (reply_port_index{message::pop<int>(msg)});
    case utils::variant_index<reply_msg, reply_task_add>(): {
        return (reply_task_add{message::pop_string(msg)});
    }
    case utils::variant_index<reply_msg, reply_transmit_function>():
        return (reply_transmit_function{
            message::pop<workers::transmit_function>(msg)});
    case utils::variant_index<reply_msg, reply_worker_ids>(): {
        return (reply_worker_ids{message::pop_vector<unsigned>(msg)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{message::pop<int>(msg)});
    }

    return (tl::make_unexpected(EINVAL));
}

} // namespace openperf::packetio::internal::api
