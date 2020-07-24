#include "api.hpp"

#include "framework/utils/variant_index.hpp"
#include "framework/utils/overloaded_visitor.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/BulkCreateMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"

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
                           [&](request::generator::create& create) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(create.data));
                           },
                           [&](request::generator::bulk::create& bulk_create) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(bulk_create.data));
                           },
                           [&](request::generator::bulk::id_list& list) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(list.data));
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
                           [&](reply::generator::list& list) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(list.data));
                           },
                           [&](reply::generator::item& item) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(item.data));
                           },
                           [&](reply::statistic::list& list) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(list.data));
                           },
                           [&](reply::statistic::item& item) {
                               return zmq_msg_init(&serialized.data,
                                                   std::move(item.data));
                           },
                           [&](const reply::info& info) {
                               return zmq_msg_init(
                                   &serialized.data, &info, sizeof(info));
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
    case utils::variant_index<api_request, request::info>():
        return request::info{};
    case utils::variant_index<api_request, request::generator::list>():
        return request::generator::list{};
    case utils::variant_index<api_request, request::generator::get>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::generator::get{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::generator::get{};
    }
    case utils::variant_index<api_request, request::generator::create>(): {
        if (zmq_msg_size(&msg.data)) {
            request::generator::create request{};
            request.data.reset(
                *zmq_msg_data<request::generator::create_data**>(&msg.data));
            return request;
        }
        return request::generator::create{};
    }
    case utils::variant_index<api_request, request::generator::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::generator::erase{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::generator::erase{};
    }
    case utils::variant_index<api_request,
                              request::generator::bulk::create>(): {
        if (zmq_msg_size(&msg.data)) {
            request::generator::bulk::create request{};
            request.data.reset(
                *zmq_msg_data<std::vector<request::generator::create_data>**>(
                    &msg.data));
            return request;
        }
        return request::generator::bulk::start{};
    }
    case utils::variant_index<api_request, request::generator::bulk::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            request::generator::bulk::erase request{};
            request.data.reset(
                *zmq_msg_data<std::vector<std::string>**>(&msg.data));
            return request;
        }
        return request::generator::bulk::stop{};
    }
    case utils::variant_index<api_request, request::generator::start>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::generator::start{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::generator::start{};
    }
    case utils::variant_index<api_request, request::generator::stop>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::generator::stop{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::generator::stop{};
    }
    case utils::variant_index<api_request, request::generator::bulk::start>(): {
        if (zmq_msg_size(&msg.data)) {
            request::generator::bulk::start request{};
            request.data.reset(
                *zmq_msg_data<std::vector<std::string>**>(&msg.data));
            return request;
        }
        return request::generator::bulk::start{};
    }
    case utils::variant_index<api_request, request::generator::bulk::stop>(): {
        if (zmq_msg_size(&msg.data)) {
            request::generator::bulk::stop request{};
            request.data.reset(
                *zmq_msg_data<std::vector<std::string>**>(&msg.data));
            return request;
        }
        return request::generator::bulk::stop{};
    }
    case utils::variant_index<api_request, request::statistic::list>():
        return request::statistic::list{};
    case utils::variant_index<api_request, request::statistic::get>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::statistic::get{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::statistic::get{};
    }
    case utils::variant_index<api_request, request::statistic::erase>(): {
        if (zmq_msg_size(&msg.data)) {
            return request::statistic::erase{
                {.id = std::string(zmq_msg_data<char*>(&msg.data),
                                   zmq_msg_size(&msg.data))}};
        }
        return request::statistic::erase{};
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
    case utils::variant_index<api_reply, reply::info>():
        return *zmq_msg_data<reply::info*>(&msg.data);
    case utils::variant_index<api_reply, reply::generator::item>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::generator::item reply{};
            reply.data.reset(
                *zmq_msg_data<reply::generator::item::item_data**>(&msg.data));
            return reply;
        }
        return reply::generator::item{};
    }
    case utils::variant_index<api_reply, reply::generator::list>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::generator::list reply{};
            reply.data.reset(
                *zmq_msg_data<std::vector<reply::generator::item::item_data>**>(
                    &msg.data));
            return reply;
        }
        return reply::generator::list{};
    }
    case utils::variant_index<api_reply, reply::statistic::item>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::statistic::item reply{};
            reply.data.reset(
                *zmq_msg_data<reply::statistic::item::item_data**>(&msg.data));
            return reply;
        }
        return reply::statistic::item{};
    }
    case utils::variant_index<api_reply, reply::statistic::list>(): {
        if (zmq_msg_size(&msg.data)) {
            reply::statistic::list reply{};
            reply.data.reset(
                *zmq_msg_data<std::vector<reply::statistic::item::item_data>**>(
                    &msg.data));
            return reply;
        }
        return reply::statistic::list{};
    }
    }
    return tl::make_unexpected(EINVAL);
}

} // namespace openperf::memory::api

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, MemoryGenerator& generator)
{
    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    auto gc = MemoryGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<MemoryGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j,
               BulkCreateMemoryGeneratorsRequest& request)
{
    request.getItems().clear();
    nlohmann::json jsonArray;
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<MemoryGenerator> newItem =
                std::make_shared<MemoryGenerator>();
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j,
               BulkDeleteMemoryGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStartMemoryGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStopMemoryGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model
