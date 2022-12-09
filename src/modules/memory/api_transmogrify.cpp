#include "api.hpp"

#include "framework/message/serialized_message.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"

namespace openperf::memory::api {

serialized_msg serialize(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request::generator::create& create) {
                     return message::push(serialized,
                                          std::move(create.generator));
                 },
                 [&](request::generator::start& start) -> int {
                     return message::push(serialized, start.id)
                            || message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(start.dynamic_results)));
                 },
                 [&](request::generator::bulk::create& create) {
                     return message::push(serialized, create.generators);
                 },
                 [&](request::generator::bulk::start& start) -> int {
                     return message::push(serialized,
                                          std::make_unique<decltype(start.ids)>(
                                              std::move(start.ids)))
                            || message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(start.dynamic_results)));
                 },
                 [&](const request::detail::empty_message&) { return (0); },
                 [&](const request::detail::id_message& msg) {
                     return message::push(serialized, msg.id);
                 },
                 [&](request::detail::ids_message& request) {
                     return message::push(
                         serialized,
                         std::make_unique<decltype(request.ids)>(
                             std::move(request.ids)));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](reply::generators& reply) {
                               return message::push(serialized,
                                                    reply.generators);
                           },
                           [&](reply::results& reply) {
                               return message::push(serialized, reply.results);
                           },
                           [&](const reply::ok&) { return 0; },
                           [&](reply::error& error) {
                               return message::push(serialized, error.info);
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
    case utils::variant_index<request_msg, request::generator::list>():
        return request::generator::list{};
    case utils::variant_index<request_msg, request::generator::get>(): {
        return request::generator::get{{.id = message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::generator::create>(): {
        auto request = request::generator::create{};
        request.generator.reset(message::pop<mem_generator_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request::generator::erase>(): {
        return request::generator::erase{{.id = message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg,
                              request::generator::bulk::create>(): {
        return request::generator::bulk::create{
            message::pop_unique_vector<mem_generator_type>(msg)};
    }
    case utils::variant_index<request_msg, request::generator::bulk::erase>(): {
        auto request = request::generator::bulk::erase{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::start>(): {
        request::generator::start request{};
        request.id = message::pop_string(msg);
        request.dynamic_results =
            std::move(*std::unique_ptr<dynamic::configuration>(
                message::pop<dynamic::configuration*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::stop>(): {
        return request::generator::stop{{.id = message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::generator::bulk::start>(): {
        request::generator::bulk::start request{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        request.dynamic_results =
            std::move(*std::unique_ptr<dynamic::configuration>(
                message::pop<dynamic::configuration*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg, request::generator::bulk::stop>(): {
        auto request = request::generator::bulk::stop{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            message::pop<decltype(request.ids)*>(msg)));
        return request;
    }
    case utils::variant_index<request_msg, request::result::list>():
        return request::result::list{};
    case utils::variant_index<request_msg, request::result::get>(): {
        return request::result::get{{.id = message::pop_string(msg)}};
    }
    case utils::variant_index<request_msg, request::result::erase>(): {
        return request::result::erase{{.id = message::pop_string(msg)}};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply::generators>(): {
        return reply::generators{
            message::pop_unique_vector<mem_generator_type>(msg)};
    }
    case utils::variant_index<reply_msg, reply::results>(): {
        return reply::results{
            message::pop_unique_vector<mem_generator_result_type>(msg)};
    }
    case utils::variant_index<reply_msg, reply::ok>():
        return reply::ok{};
    case utils::variant_index<reply_msg, reply::error>():
        return reply::error{message::pop<reply::typed_error>(msg)};
    }

    return tl::make_unexpected(EINVAL);
}

} // namespace openperf::memory::api
