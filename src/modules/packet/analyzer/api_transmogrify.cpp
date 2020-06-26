#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/analyzer/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketAnalyzer.h"
#include "swagger/v1/model/PacketAnalyzerResult.h"
#include "swagger/v1/model/RxFlow.h"

namespace openperf::packet::analyzer::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_analyzers& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](request_create_analyzer& request) {
                     return (message::push(serialized,
                                           std::move(request.analyzer)));
                 },
                 [&](const request_delete_analyzers&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_analyzer& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_analyzer& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_reset_analyzer& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_start_analyzer& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_stop_analyzer& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_bulk_create_analyzers& request) {
                     return (message::push(serialized, request.analyzers));
                 },
                 [&](request_bulk_delete_analyzers& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_start_analyzers& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_stop_analyzers& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_list_analyzer_results& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_delete_analyzer_results&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_analyzer_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_analyzer_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_list_rx_flows& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_get_rx_flow& request) {
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
                 [&](reply_analyzers& reply) {
                     return (message::push(serialized, reply.analyzers));
                 },
                 [&](reply_analyzer_results& reply) {
                     return (message::push(serialized, reply.analyzer_results));
                 },
                 [&](reply_rx_flows& reply) {
                     return (message::push(serialized, reply.flows));
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
    case utils::variant_index<request_msg, request_list_analyzers>(): {
        auto request = request_list_analyzers{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_analyzer>(): {
        auto request = request_create_analyzer{};
        request.analyzer.reset(message::pop<analyzer_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_analyzers>():
        return (request_delete_analyzers{});
    case utils::variant_index<request_msg, request_get_analyzer>(): {
        return (request_get_analyzer{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_analyzer>(): {
        return (request_delete_analyzer{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_reset_analyzer>(): {
        return (request_reset_analyzer{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_start_analyzer>(): {
        return (request_start_analyzer{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_stop_analyzer>(): {
        return (request_stop_analyzer{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_create_analyzers>(): {
        return (request_bulk_create_analyzers{
            message::pop_unique_vector<analyzer_type>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_delete_analyzers>(): {
        return (request_bulk_delete_analyzers{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_start_analyzers>(): {
        return (request_bulk_start_analyzers{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_stop_analyzers>(): {
        return (request_bulk_stop_analyzers{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_list_analyzer_results>(): {
        auto request = request_list_analyzer_results{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_analyzer_results>():
        return (request_delete_analyzer_results{});
    case utils::variant_index<request_msg, request_get_analyzer_result>(): {
        return (request_get_analyzer_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_analyzer_result>(): {
        return (request_delete_analyzer_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_list_rx_flows>(): {
        auto request = request_list_rx_flows{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_rx_flow>(): {
        return (request_get_rx_flow{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_analyzers>(): {
        return (
            reply_analyzers{message::pop_unique_vector<analyzer_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_analyzer_results>(): {
        return (reply_analyzer_results{
            message::pop_unique_vector<analyzer_result_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_rx_flows>(): {
        return (reply_rx_flows{message::pop_unique_vector<rx_flow_type>(msg)});
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

} // namespace openperf::packet::analyzer::api

namespace swagger::v1::model {
void from_json(const nlohmann::json& j, PacketAnalyzer& analyzer)
{
    /*
     * We can't call analyzer's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        analyzer.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        analyzer.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        analyzer.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketAnalyzerConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        analyzer.setConfig(config);
    }
}

} // namespace swagger::v1::model
