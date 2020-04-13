#include "memory/api.hpp"
#include "utils/variant_index.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::memory::api {

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
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return error;
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
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

serialized_msg serialize(const api_request& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request::generator::create& cfg) {
                     return zmq_msg_init(&serialized.data, &cfg, sizeof(cfg));
                 },
                 [&](const id_list& list) {
                     return zmq_msg_init(&serialized.data,
                                         list.data(),
                                         sizeof(id_t) * list.size());
                 },
                 [&](const id_message& id) {
                     return zmq_msg_init(
                         &serialized.data, (const char*)&id.id, sizeof(id.id));
                 },
                 [&](const message&) {
                     return zmq_msg_init(&serialized.data);
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize(const api_reply& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const reply::generator::list& list) {
                     return zmq_msg_init(&serialized.data,
                                         list.data(),
                                         sizeof(reply::generator::item)
                                             * list.size());
                 },
                 [&](const reply::generator::item& item) {
                     return zmq_msg_init(&serialized.data, &item, sizeof(item));
                 },
                 [&](const reply::statistic::list& list) {
                     return zmq_msg_init(&serialized.data,
                                         list.data(),
                                         sizeof(reply::statistic::item)
                                             * list.size());
                 },
                 [&](const reply::statistic::item& item) {
                     return zmq_msg_init(&serialized.data, &item, sizeof(item));
                 },
                 [&](const reply::info& info) {
                     return zmq_msg_init(&serialized.data, &info, sizeof(info));
                 },
                 [&](const reply::error&) {
                     return zmq_msg_init(&serialized.data);
                 },
                 [&](const message&) {
                     return zmq_msg_init(&serialized.data);
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<api_request, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<api_request>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<api_request, request::info>():
        return request::info{};
    case utils::variant_index<api_request, request::generator::list>():
        return request::generator::list{};
    case utils::variant_index<api_request, request::generator::get>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::generator::get{{.id = std::string_view(id)}};
        }
        return request::generator::get{};
    }
    case utils::variant_index<api_request, request::generator::create>(): {
        if (zmq_msg_size(&msg.data)) {
            return *zmq_msg_data<request::generator::create*>(&msg.data);
        }
        return request::generator::create{};
    }
    case utils::variant_index<api_request, request::generator::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::generator::erase{{.id = std::string_view(id)}};
        }
        return request::generator::erase{};
    }
    case utils::variant_index<api_request, request::generator::start>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::generator::start{{.id = std::string_view(id)}};
        }
        return request::generator::start{};
    }
    case utils::variant_index<api_request, request::generator::stop>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::generator::stop{{.id = std::string_view(id)}};
        }
        return request::generator::stop{};
    }
    case utils::variant_index<api_request, request::generator::bulk::start>(): {
        auto size = zmq_msg_size<id_t>(&msg.data);
        auto array = zmq_msg_data<id_t*>(&msg.data);

        request::generator::bulk::stop list;
        for (size_t i = 0; i < size; ++i) list.push_back(array[i]);
        return list;
    }
    case utils::variant_index<api_request, request::generator::bulk::stop>(): {
        auto size = zmq_msg_size<id_t>(&msg.data);
        auto array = zmq_msg_data<id_t*>(&msg.data);

        request::generator::bulk::stop list;
        for (size_t i = 0; i < size; ++i) list.push_back(array[i]);
        return list;
    }
    case utils::variant_index<api_request, request::statistic::list>():
        return request::statistic::list{};
    case utils::variant_index<api_request, request::statistic::get>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::statistic::get{{.id = std::string_view(id)}};
        }
        return request::statistic::get{};
    }
    case utils::variant_index<api_request, request::statistic::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            auto id = zmq_msg_data<char*>(&msg.data);
            return request::statistic::erase{{.id = std::string_view(id)}};
        }
        return request::statistic::erase{};
    }
    }

    return (tl::make_unexpected(EINVAL));
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
    case utils::variant_index<api_reply, reply::info>():
        return *zmq_msg_data<reply::info*>(&msg.data);
    case utils::variant_index<api_reply, reply::generator::item>():
        return *zmq_msg_data<reply::generator::item*>(&msg.data);
    case utils::variant_index<api_reply, reply::generator::list>(): {
        auto size = zmq_msg_size<reply::generator::item>(&msg.data);
        auto array = zmq_msg_data<reply::generator::item*>(&msg.data);

        reply::generator::list list;
        for (size_t i = 0; i < size; ++i) list.push_back(array[i]);
        return list;
    }
    case utils::variant_index<api_reply, reply::statistic::item>():
        return *zmq_msg_data<reply::statistic::item*>(&msg.data);
    case utils::variant_index<api_reply, reply::statistic::list>(): {
        auto size = zmq_msg_size<reply::statistic::item>(&msg.data);
        auto array = zmq_msg_data<reply::statistic::item*>(&msg.data);

        reply::statistic::list list;
        for (size_t i = 0; i < size; ++i) list.push_back(array[i]);
        return list;
    }
    }
    return (tl::make_unexpected(EINVAL));
}

} // namespace openperf::memory::api
