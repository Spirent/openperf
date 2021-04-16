#include <sstream>

#include "framework/message/serialized_message.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"
#include "network/api.hpp"
#include "network/models/generator.hpp"
#include "network/models/generator_result.hpp"
#include "network/models/server.hpp"

namespace openperf::network::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (openperf::message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request::generator::create& network_generator) {
                     return openperf::message::push(
                         serialized, std::move(network_generator.source));
                 },
                 [&](request::generator::bulk::create& request) {
                     return openperf::message::push(serialized,
                                                    request.generators);
                 },
                 [&](request::generator::start& request) -> int {
                     return openperf::message::push(serialized, request.id)
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(request.dynamic_results)));
                 },
                 [&](request::generator::bulk::start& request) -> int {
                     return openperf::message::push(
                                serialized,
                                std::make_unique<decltype(request.ids)>(
                                    std::move(request.ids)))
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(request.dynamic_results)));
                 },
                 [&](request::server::create& network_server) {
                     return openperf::message::push(
                         serialized, std::move(network_server.source));
                 },
                 [&](request::server::bulk::create& request) {
                     return openperf::message::push(serialized,
                                                    request.servers);
                 },
                 [&](request::generator::toggle& request) -> int {
                     return openperf::message::push(serialized,
                                                    std::move(request.ids))
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(request.dynamic_results)));
                 },
                 [&](id_message& msg) {
                     return openperf::message::push(serialized, msg.id);
                 },
                 [&](id_list_message& msg) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<id_list_message>(std::move(msg)));
                 },
                 [&](const message&) { return 0; }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (openperf::message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](reply::generator::list& reply) {
                     return openperf::message::push(serialized,
                                                    reply.generators);
                 },
                 [&](reply::statistic::list& reply) {
                     return openperf::message::push(serialized, reply.results);
                 },
                 [&](reply::server::list& reply) {
                     return openperf::message::push(serialized, reply.servers);
                 },
                 [&](const reply::ok&) { return 0; },
                 [&](reply::error& error) {
                     return openperf::message::push(serialized,
                                                    std::move(error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return serialized;
}

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = openperf::message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request::generator::list>(): {
        return request::generator::list{};
    }
    case utils::variant_index<request_msg, request::generator::create>(): {
        auto request = request::generator::create{};
        request.source.reset(openperf::message::pop<model::generator*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::get>(): {
        return request::generator::get{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::generator::erase>(): {
        return request::generator::erase{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg,
                              request::generator::bulk::create>(): {
        return request::generator::bulk::create{
            openperf::message::pop_unique_vector<model::generator>(msg)};
    }
    case utils::variant_index<request_msg, request::generator::bulk::erase>(): {
        return std::move(
            *openperf::message::pop<request::generator::bulk::erase*>(msg));
    }
    case utils::variant_index<request_msg, request::generator::start>(): {
        auto request = request::generator::start{};
        request.id = openperf::message::pop_string(msg);
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            openperf::message::pop<dynamic::configuration*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::stop>(): {
        return request::generator::stop{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::generator::bulk::start>(): {
        auto request = request::generator::bulk::start{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            openperf::message::pop<decltype(request.ids)*>(msg)));
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            openperf::message::pop<dynamic::configuration*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::bulk::stop>(): {
        return std::move(
            *openperf::message::pop<request::generator::bulk::stop*>(msg));
    }
    case utils::variant_index<request_msg, request::generator::toggle>(): {
        auto request = request::generator::toggle{};
        using ptr = decltype(request::generator::toggle::ids)::pointer;
        request.ids.reset(openperf::message::pop<ptr>(msg));
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            openperf::message::pop<dynamic::configuration*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request::statistic::list>(): {
        return request::statistic::list{};
    }
    case utils::variant_index<request_msg, request::statistic::get>(): {
        return request::statistic::get{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::statistic::erase>(): {
        return request::statistic::erase{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::server::list>(): {
        return request::server::list{};
    }
    case utils::variant_index<request_msg, request::server::create>(): {
        auto request = request::server::create{};
        request.source.reset(openperf::message::pop<model::server*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request::server::get>(): {
        return request::server::get{{.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::server::erase>(): {
        return request::server::erase{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::server::bulk::create>(): {
        return request::server::bulk::create{
            openperf::message::pop_unique_vector<model::server>(msg)};
    }
    case utils::variant_index<request_msg, request::server::bulk::erase>(): {
        return std::move(
            *openperf::message::pop<request::server::bulk::erase*>(msg));
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = openperf::message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply::generator::list>(): {
        return reply::generator::list{
            openperf::message::pop_unique_vector<model::generator>(msg)};
    }
    case utils::variant_index<reply_msg, reply::statistic::list>(): {
        return reply::statistic::list{
            openperf::message::pop_unique_vector<model::generator_result>(msg)};
    }
    case utils::variant_index<reply_msg, reply::server::list>(): {
        return reply::server::list{
            openperf::message::pop_unique_vector<model::server>(msg)};
    }
    case utils::variant_index<reply_msg, reply::ok>():
        return reply::ok{};
    case utils::variant_index<reply_msg, reply::error>():
        auto reply = reply::error{};
        reply.info.reset(openperf::message::pop<reply_error*>(msg));
        return reply;
    }

    return tl::make_unexpected(EINVAL);
}

std::string to_string(const api::reply_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return "";
    case api::error_type::ZMQ_ERROR:
        return zmq_strerror(error.code);
    case api::error_type::CUSTOM_ERROR:
        return error.message;
    default:
        return "unknown error type";
    }
}

reply::error to_error(error_type type, int code, const std::string& message)
{
    return reply::error{.info = std::make_unique<reply_error>(reply_error{
                            .type = type, .code = code, .message = message})};
}

} // namespace openperf::network::api
