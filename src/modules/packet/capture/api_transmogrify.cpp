#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/capture/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketCapture.h"
#include "swagger/v1/model/PacketCaptureResult.h"

namespace openperf::packet::capture::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_captures& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](request_create_capture& request) {
                     return (
                         message::push(serialized, std::move(request.capture)));
                 },
                 [&](const request_delete_captures&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_capture& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_capture& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_start_capture& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_stop_capture& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_bulk_create_captures& request) {
                     return (message::push(serialized, request.captures));
                 },
                 [&](request_bulk_delete_captures& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_start_captures& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_stop_captures& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_list_capture_results& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_delete_capture_results&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_capture_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_capture_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_create_capture_transfer& request) -> int {
                     return (message::push(serialized,
                                           request.id.data(),
                                           request.id.length())
                             || message::push(serialized, request.transfer));
                 },
                 [&](const request_delete_capture_transfer& request) {
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
                 [&](reply_captures& reply) {
                     return (message::push(serialized, reply.captures));
                 },
                 [&](reply_capture_results& reply) {
                     return (message::push(serialized, reply.capture_results));
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
    case utils::variant_index<request_msg, request_list_captures>(): {
        auto request = request_list_captures{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_capture>(): {
        auto request = request_create_capture{};
        request.capture.reset(message::pop<capture_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_captures>():
        return (request_delete_captures{});
    case utils::variant_index<request_msg, request_get_capture>(): {
        return (request_get_capture{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_capture>(): {
        return (request_delete_capture{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_start_capture>(): {
        return (request_start_capture{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_stop_capture>(): {
        return (request_stop_capture{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_create_captures>(): {
        return (request_bulk_create_captures{
            message::pop_unique_vector<capture_type>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_delete_captures>(): {
        return (request_bulk_delete_captures{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_start_captures>(): {
        return (request_bulk_start_captures{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_stop_captures>(): {
        return (request_bulk_stop_captures{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_list_capture_results>(): {
        auto request = request_list_capture_results{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_capture_results>():
        return (request_delete_capture_results{});
    case utils::variant_index<request_msg, request_get_capture_result>(): {
        return (request_get_capture_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_capture_result>(): {
        return (request_delete_capture_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_create_capture_transfer>(): {
        auto request = request_create_capture_transfer{
            .id = message::pop_string(msg),
            .transfer = message::pop<transfer_context*>(msg)};
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_capture_transfer>(): {
        return (request_delete_capture_transfer{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_captures>(): {
        return (reply_captures{message::pop_unique_vector<capture_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_capture_results>(): {
        return (reply_capture_results{
            message::pop_unique_vector<capture_result_type>(msg)});
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

} // namespace openperf::packet::capture::api

namespace swagger::v1::model {
void from_json(const nlohmann::json& j, PacketCapture& capture)
{
    /*
     * We can't call capture's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        capture.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        capture.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        capture.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketCaptureConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        capture.setConfig(config);
    }
}

} // namespace swagger::v1::model
