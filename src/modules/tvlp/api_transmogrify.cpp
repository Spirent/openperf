#include "api.hpp"

#include "framework/utils/variant_index.hpp"
#include "framework/utils/overloaded_visitor.hpp"

namespace openperf::tvlp::api {

serialized_msg serialize(api_request&& msg)
{
    serialized_msg serialized;
    auto error = (openperf::message::push(serialized, msg.index())
                  || std::visit(utils::overloaded_visitor(
                                    [&](request::tvlp::create& create) {
                                        return openperf::message::push(
                                            serialized, std::move(create.data));
                                    },
                                    [&](const id_message& msg) {
                                        return openperf::message::push(
                                            serialized, msg.id);
                                    },
                                    [&](const message&) { return 0; }),
                                msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize(api_reply&& msg)
{
    serialized_msg serialized;
    auto error = (openperf::message::push(serialized, msg.index())
                  || std::visit(utils::overloaded_visitor(
                                    [&](reply::tvlp::list& reply) {
                                        return openperf::message::push(
                                            serialized, std::move(reply.data));
                                    },
                                    [&](reply::tvlp::item& reply) {
                                        return openperf::message::push(
                                            serialized, std::move(reply.data));
                                    },
                                    [&](reply::error& reply) {
                                        return openperf::message::push(
                                            serialized, std::move(reply.data));
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
        auto id = openperf::message::pop_string(msg);
        if (id.length()) { return request::tvlp::get{{.id = std::move(id)}}; }
        return request::tvlp::get{};
    }
    case utils::variant_index<api_request, request::tvlp::create>(): {
        request::tvlp::create request{};
        request.data.reset(openperf::message::pop<tvlp_config_t*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::tvlp::erase>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) { return request::tvlp::erase{{.id = std::move(id)}}; }
        return request::tvlp::erase{};
    }
    case utils::variant_index<api_request, request::tvlp::start>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) { return request::tvlp::start{{.id = std::move(id)}}; }
        return request::tvlp::start{};
    }
    case utils::variant_index<api_request, request::tvlp::stop>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) { return request::tvlp::stop{{.id = std::move(id)}}; }
        return request::tvlp::stop{};
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
        reply::error reply{};
        reply.data.reset(openperf::message::pop<reply::error_data*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::tvlp::item>(): {
        reply::tvlp::item reply{};
        reply.data.reset(openperf::message::pop<tvlp_config_t*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::tvlp::list>(): {
        reply::tvlp::list reply{};
        reply.data.reset(
            openperf::message::pop<std::vector<tvlp_config_t>*>(msg));
        return reply;
    }
    }
    return tl::make_unexpected(EINVAL);
}

} // namespace openperf::tvlp::api
