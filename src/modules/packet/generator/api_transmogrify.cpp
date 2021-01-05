#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/generator/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/PacketGeneratorLearningResults.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_generators& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](request_create_generator& request) {
                     return (message::push(serialized,
                                           std::move(request.generator)));
                 },
                 [&](const request_delete_generators&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_start_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_stop_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_bulk_create_generators& request) {
                     return (message::push(serialized, request.generators));
                 },
                 [&](request_bulk_delete_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_start_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_stop_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_toggle_generator& request) {
                     return (message::push(serialized, std::move(request.ids)));
                 },
                 [&](request_list_generator_results& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_delete_generator_results&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_generator_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_generator_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_list_tx_flows& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_get_tx_flow& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_get_learning_results& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_retry_learning& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_start_learning& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_stop_learning& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
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
                 [&](reply_generators& reply) {
                     return (message::push(serialized, reply.generators));
                 },
                 [&](reply_generator_results& reply) {
                     return (
                         message::push(serialized, reply.generator_results));
                 },
                 [&](reply_tx_flows& reply) {
                     return (message::push(serialized, reply.flows));
                 },
                 [&](reply_learning_results& reply) {
                     return (message::push(serialized, reply.results));
                 },
                 [&](const reply_ok&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const reply_error& error) {
                     return (message::push(serialized, error.info));
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
    case utils::variant_index<request_msg, request_list_generators>(): {
        auto request = request_list_generators{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_generator>(): {
        auto request = request_create_generator{};
        request.generator.reset(message::pop<generator_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generators>():
        return (request_delete_generators{});
    case utils::variant_index<request_msg, request_get_generator>(): {
        return (request_get_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_generator>(): {
        return (request_delete_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_start_generator>(): {
        return (request_start_generator({message::pop_string(msg)}));
    }
    case utils::variant_index<request_msg, request_stop_generator>(): {
        return (request_stop_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_create_generators>(): {
        return (request_bulk_create_generators{
            message::pop_unique_vector<generator_type>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_delete_generators>(): {
        return (request_bulk_delete_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_start_generators>(): {
        return (request_bulk_start_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_stop_generators>(): {
        return (request_bulk_stop_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_toggle_generator>(): {
        auto request = request_toggle_generator{};
        using ptr = decltype(request_toggle_generator::ids)::pointer;
        request.ids.reset(message::pop<ptr>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_list_generator_results>(): {
        auto request = request_list_generator_results{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generator_results>():
        return (request_delete_generator_results{});
    case utils::variant_index<request_msg, request_get_generator_result>(): {
        return (request_get_generator_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_generator_result>(): {
        return (request_delete_generator_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_list_tx_flows>(): {
        auto request = request_list_tx_flows{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_tx_flow>(): {
        return (request_get_tx_flow{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_get_learning_results>(): {
        return (request_get_learning_results{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_retry_learning>(): {
        return (request_retry_learning{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_start_learning>(): {
        return (request_start_learning{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_stop_learning>(): {
        return (request_stop_learning{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_generators>(): {
        return (
            reply_generators{message::pop_unique_vector<generator_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_generator_results>(): {
        return (reply_generator_results{
            message::pop_unique_vector<generator_result_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_tx_flows>(): {
        return (reply_tx_flows{message::pop_unique_vector<tx_flow_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_learning_results>(): {
        return (reply_learning_results{
            message::pop_unique_vector<learning_results_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{message::pop<typed_error>(msg)});
    }

    return (tl::make_unexpected(EINVAL));
}

reply_error to_error(error_type type, int value)
{
    return (reply_error{.info = typed_error{.type = type, .value = value}});
}

} // namespace openperf::packet::generator::api
