#include <sstream>

#include "api.hpp"

#include "framework/message/serialized_message.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"

namespace openperf::cpu::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_cpu_generator_list&) { return 0; },
                 [&](const request_cpu_generator& cpu_generator) {
                     return message::push(serialized, cpu_generator.id);
                 },
                 [&](request_cpu_generator_add& cpu_generator) {
                     return message::push(serialized,
                                          std::move(cpu_generator.source));
                 },
                 [&](const request_cpu_generator_del& cpu_generator) {
                     return message::push(serialized, cpu_generator.id);
                 },
                 [&](request_cpu_generator_bulk_add& request) {
                     return message::push(serialized, request.generators);
                 },
                 [&](request_cpu_generator_bulk_del& request) {
                     return message::push(
                         serialized,
                         std::make_unique<decltype(request.ids)>(
                             std::move(request.ids)));
                 },
                 [&](request_cpu_generator_start& request) -> int {
                     return message::push(serialized, request.id)
                            || message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(request.dynamic_results)));
                 },
                 [&](const request_cpu_generator_stop& cpu_generator) {
                     return message::push(serialized, cpu_generator.id);
                 },
                 [&](request_cpu_generator_bulk_start& request) -> int {
                     return message::push(
                                serialized,
                                std::make_unique<decltype(request.ids)>(
                                    std::move(request.ids)))
                            || message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(request.dynamic_results)));
                 },
                 [&](request_cpu_generator_bulk_stop& request) {
                     return message::push(
                         serialized,
                         std::make_unique<decltype(request.ids)>(
                             std::move(request.ids)));
                 },
                 [&](const request_cpu_generator_result_list&) { return 0; },
                 [&](const request_cpu_generator_result& result) {
                     return message::push(serialized, result.id);
                 },
                 [&](const request_cpu_generator_result_del& result) {
                     return message::push(serialized, result.id);
                 },
                 [&](const request_cpu_info&) { return 0; }),
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
                 [&](reply_cpu_generators& reply) {
                     return message::push(serialized, reply.generators);
                 },
                 [&](reply_cpu_generator_results& reply) {
                     return message::push(serialized, reply.results);
                 },
                 [&](reply_cpu_info& reply) {
                     return message::push(serialized, std::move(reply.info));
                 },
                 [&](const reply_ok&) { return 0; },
                 [&](reply_error& error) {
                     return message::push(serialized, std::move(error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return serialized;
}

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request_cpu_generator_list>(): {
        return request_cpu_generator_list{};
    }
    case utils::variant_index<request_msg, request_cpu_generator_add>(): {
        auto request = request_cpu_generator_add{};
        request.source.reset(message::pop<cpu_generator_t*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator>(): {
        return request_cpu_generator{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_del>(): {
        return request_cpu_generator_del{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_add>(): {
        return request_cpu_generator_bulk_add{
            message::pop_unique_vector<cpu_generator_t>(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_del>(): {
        auto request = request_cpu_generator_bulk_del{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator_start>(): {
        auto request = request_cpu_generator_start{};
        request.id = message::pop_string(msg);
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            message::pop<dynamic::configuration*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator_stop>(): {
        return request_cpu_generator_stop{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_start>(): {
        auto request = request_cpu_generator_bulk_start{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            message::pop<dynamic::configuration*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_stop>(): {
        auto request = request_cpu_generator_bulk_stop{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_list>(): {
        return request_cpu_generator_result_list{};
    }
    case utils::variant_index<request_msg, request_cpu_generator_result>(): {
        return request_cpu_generator_result{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_del>(): {
        return request_cpu_generator_result_del{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_info>(): {
        return request_cpu_info{};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_cpu_generators>(): {
        return reply_cpu_generators{
            message::pop_unique_vector<cpu_generator_t>(msg)};
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_results>(): {
        return reply_cpu_generator_results{
            message::pop_unique_vector<cpu_generator_result_t>(msg)};
    }
    case utils::variant_index<reply_msg, reply_cpu_info>(): {
        auto reply = reply_cpu_info{};
        reply.info.reset(message::pop<cpu_info_t*>(msg));
        return reply;
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return reply_ok{};
    case utils::variant_index<reply_msg, reply_error>():
        auto reply = reply_error{};
        reply.info.reset(message::pop<typed_error*>(msg));
        return reply;
    }

    return tl::make_unexpected(EINVAL);
}

std::string to_string(const api::typed_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return "";
    case api::error_type::ZMQ_ERROR:
        return zmq_strerror(error.code);
    case api::error_type::CUSTOM_ERROR:
        return error.value;
    default:
        return "unknown error type";
    }
}

reply_error to_error(error_type type, int code, const std::string& value)
{
    return reply_error{.info = std::make_unique<typed_error>(typed_error{
                           .type = type, .code = code, .value = value})};
}

} // namespace openperf::cpu::api
