#include "api.hpp"

#include "message/serialized_message.hpp"
#include "framework/utils/variant_index.hpp"
#include "framework/utils/overloaded_visitor.hpp"

namespace openperf::tvlp::api {

serialized_msg serialize(api_request&& msg)
{
    serialized_msg serialized;
    auto error =
        (openperf::message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request::tvlp::create& create) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<request::tvlp::create>(
                             std::move(create)));
                 },
                 [&](request::tvlp::start& start) -> int {
                     return openperf::message::push(serialized, start.id)
                            || openperf::message::push(serialized,
                                                       start.start_time)
                            || openperf::message::push(
                                serialized,
                                std::make_unique<decltype(
                                    start.dynamic_results)>(
                                    std::move(start.dynamic_results)));
                 },
                 [&](const id_message& msg) {
                     return openperf::message::push(serialized, msg.id);
                 },
                 [&](const message&) { return 0; }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize(api_reply&& msg)
{
    serialized_msg serialized;
    auto error =
        (openperf::message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](reply::tvlp::list& list) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::tvlp::list>(std::move(list)));
                 },
                 [&](reply::tvlp::item& item) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::tvlp::item>(std::move(item)));
                 },
                 [&](reply::tvlp::result::list& list) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::tvlp::result::list>(
                             std::move(list)));
                 },
                 [&](reply::tvlp::result::item& item) {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::tvlp::result::item>(
                             std::move(item)));
                 },
                 [&](reply::error& error) -> int {
                     return openperf::message::push(serialized, error.type)
                            || openperf::message::push(serialized, error.code)
                            || openperf::message::push(serialized,
                                                       error.message);
                 },
                 [&](const message&) { return 0; }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return serialized;
}

tl::expected<api_request, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<api_request>().index());
    auto idx = openperf::message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<api_request, request::tvlp::list>():
        return request::tvlp::list{};
    case utils::variant_index<api_request, request::tvlp::get>(): {
        return request::tvlp::get{{.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::tvlp::create>(): {
        return std::move(*std::unique_ptr<request::tvlp::create>(
            openperf::message::pop<request::tvlp::create*>(msg)));
    }
    case utils::variant_index<api_request, request::tvlp::erase>(): {
        return request::tvlp::erase{{.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::tvlp::start>(): {
        request::tvlp::start request{};
        request.id = openperf::message::pop_string(msg);
        request.start_time = openperf::message::pop<time_point>(msg);
        request.dynamic_results =
            std::move(*std::unique_ptr<decltype(request.dynamic_results)>(
                openperf::message::pop<decltype(request.dynamic_results)*>(
                    msg)));
        return request;
    }
    case utils::variant_index<api_request, request::tvlp::stop>(): {
        return request::tvlp::stop{{.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::tvlp::result::list>():
        return request::tvlp::result::list{};
    case utils::variant_index<api_request, request::tvlp::result::get>(): {
        return request::tvlp::result::get{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::tvlp::result::erase>(): {
        return request::tvlp::result::erase{
            {.id = openperf::message::pop_string(msg)}};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<api_reply, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<api_reply>().index());
    auto idx = openperf::message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<api_reply, reply::ok>(): {
        return reply::ok{};
    }
    case utils::variant_index<api_reply, reply::error>(): {
        reply::error error{};
        error.type = openperf::message::pop<reply::error::type_t>(msg);
        error.code = openperf::message::pop<int>(msg);
        error.message = openperf::message::pop_string(msg);
        return error;
    }
    case utils::variant_index<api_reply, reply::tvlp::item>(): {
        return std::move(*std::unique_ptr<reply::tvlp::item>(
            openperf::message::pop<reply::tvlp::item*>(msg)));
    }
    case utils::variant_index<api_reply, reply::tvlp::list>(): {
        return std::move(*std::unique_ptr<reply::tvlp::list>(
            openperf::message::pop<reply::tvlp::list*>(msg)));
    }
    case utils::variant_index<api_reply, reply::tvlp::result::item>(): {
        return std::move(*std::unique_ptr<reply::tvlp::result::item>(
            openperf::message::pop<reply::tvlp::result::item*>(msg)));
    }
    case utils::variant_index<api_reply, reply::tvlp::result::list>(): {
        return std::move(*std::unique_ptr<reply::tvlp::result::list>(
            openperf::message::pop<reply::tvlp::result::list*>(msg)));
    }
    }
    return tl::make_unexpected(EINVAL);
}

} // namespace openperf::tvlp::api
