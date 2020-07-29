#include "api.hpp"

#include "framework/utils/variant_index.hpp"
#include "framework/utils/overloaded_visitor.hpp"

#include "swagger/v1/model/TvlpConfiguration.h"

namespace openperf::tvlp::api {

static void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

int send_message(void* socket, serialized_msg&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);

        return errno;
    }

    return 0;
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1 || zmq_msg_init(&msg.data) == -1) {
        close(msg);

        return tl::make_unexpected(ENOMEM);
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);

        return tl::make_unexpected(errno);
    }

    return msg;
}

template <typename T> static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return error;
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return 0;
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, std::unique_ptr<T> value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(msg));
    *ptr = value.release();
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return error;
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy_n(src, length, ptr);
    return 0;
}

template <typename T> static T zmq_msg_data(const zmq_msg_t* msg)
{
    return reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg)));
}

template <typename T> static size_t zmq_msg_size(const zmq_msg_t* msg)
{
    return zmq_msg_size(const_cast<zmq_msg_t*>(msg)) / sizeof(T);
}

serialized_msg serialize(api_request&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](request::tvlp::create& create) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(create.data));
                           },
                           [&](const id_message& msg) {
                               return zmq_msg_init(&serialized.data,
                                                   msg.id.data(),
                                                   msg.id.length());
                           },
                           [&](const message&) {
                               return zmq_msg_init(&serialized.data);
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize(api_reply&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](reply::tvlp::list& reply) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(reply.data)));
                           },
                           [&](const reply::error& error) {
                               return zmq_msg_init(
                                   &serialized.data, &error, sizeof(error));
                           },
                           [&](const message&) {
                               return zmq_msg_init(&serialized.data);
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return serialized;
}

tl::expected<api_request, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<api_request>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<api_request, request::tvlp::list>():
        return request::tvlp::list{};
    case utils::variant_index<api_request, request::tvlp::get>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::tvlp::get{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::tvlp::get{};
    }
    case utils::variant_index<api_request, request::tvlp::create>(): {
        if (zmq_msg_size(&msg.data)) {
            request::tvlp::create request{};
            request.data.reset(*zmq_msg_data<tvlp_config_t**>(&msg.data));
            return request;
        }
        return request::tvlp::create{};
    }
    case utils::variant_index<api_request, request::tvlp::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::tvlp::erase{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::tvlp::erase{};
    }
    case utils::variant_index<api_request, request::tvlp::start>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::tvlp::start{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::tvlp::start{};
    }
    case utils::variant_index<api_request, request::tvlp::stop>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::tvlp::stop{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::tvlp::stop{};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<api_reply, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<api_reply>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<api_reply, reply::ok>():
        return reply::ok{};
    case utils::variant_index<api_reply, reply::error>():
        return *zmq_msg_data<reply::error*>(&msg.data);
    case utils::variant_index<api_reply, reply::tvlp::item>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::tvlp::item reply{};
            reply.data.reset(*zmq_msg_data<tvlp_config_t**>(&msg.data));
            return reply;
        }
        return reply::tvlp::item{};
    }
    case utils::variant_index<api_reply, reply::tvlp::list>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::tvlp::list reply{};
            reply.data.reset(
                *zmq_msg_data<std::vector<tvlp_config_t>**>(&msg.data));
            return reply;
        }
        return reply::tvlp::list{};
    }
    }
    return tl::make_unexpected(EINVAL);
}

} // namespace openperf::tvlp::api

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, TvlpConfiguration& generator)
{
    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
}

} // namespace swagger::v1::model
