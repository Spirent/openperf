#include "api.hpp"

#include "framework/utils/variant_index.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "message/serialized_message.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/BulkCreateMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"

namespace openperf::memory::api {

serialized_msg serialize(api_request&& msg)
{
    serialized_msg serialized;
    auto error =
        (openperf::message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request::generator::create& create) {
                     return openperf::message::push(serialized,
                                                    std::move(create.data));
                 },
                 [&](request::generator::start& start) {
                     return openperf::message::push(serialized,
                                                    std::move(start.data));
                 },
                 [&](request::generator::bulk::create& bulk_create) {
                     return openperf::message::push(
                         serialized, std::move(bulk_create.data));
                 },
                 [&](request::generator::bulk::start& start) {
                     return openperf::message::push(serialized,
                                                    std::move(start.data));
                 },
                 [&](request::generator::bulk::id_list& list) {
                     return openperf::message::push(serialized,
                                                    std::move(list.data));
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
         || std::visit(utils::overloaded_visitor(
                           [&](reply::generator::list& list) {
                               return openperf::message::push(
                                   serialized, std::move(list.data));
                           },
                           [&](reply::generator::item& item) {
                               return openperf::message::push(
                                   serialized, std::move(item.data));
                           },
                           [&](reply::statistic::list& list) {
                               return openperf::message::push(
                                   serialized, std::move(list.data));
                           },
                           [&](reply::statistic::item& item) {
                               return openperf::message::push(
                                   serialized, std::move(item.data));
                           },
                           [&](const reply::info& info) {
                               return openperf::message::push(serialized, info);
                           },
                           [&](reply::error& error) {
                               return openperf::message::push(
                                   serialized, std::move(error.data));
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
    case utils::variant_index<api_request, request::info>():
        return request::info{};
    case utils::variant_index<api_request, request::generator::list>():
        return request::generator::list{};
    case utils::variant_index<api_request, request::generator::get>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) {
            return request::generator::get{{.id = std::move(id)}};
        }
        return request::generator::get{};
    }
    case utils::variant_index<api_request, request::generator::create>(): {
        request::generator::create request{};
        request.data.reset(
            openperf::message::pop<request::generator::create_data*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::generator::erase>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) {
            return request::generator::erase{{.id = std::move(id)}};
        }
        return request::generator::erase{};
    }
    case utils::variant_index<api_request,
                              request::generator::bulk::create>(): {
        request::generator::bulk::create request{};
        request.data.reset(openperf::message::pop<
                           std::vector<request::generator::create_data>*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::generator::bulk::erase>(): {
        request::generator::bulk::erase request{};
        request.data.reset(
            openperf::message::pop<std::vector<std::string>*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::generator::start>(): {
        request::generator::start request{};
        request.data.reset(
            openperf::message::pop<request::generator::start::start_data*>(
                msg));
        return request;
    }
    case utils::variant_index<api_request, request::generator::stop>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) {
            return request::generator::stop{{.id = std::move(id)}};
        }
        return request::generator::stop{};
    }
    case utils::variant_index<api_request, request::generator::bulk::start>(): {
        request::generator::bulk::start request{};
        request.data.reset(openperf::message::pop<
                           request::generator::bulk::start::start_data*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::generator::bulk::stop>(): {
        request::generator::bulk::stop request{};
        request.data.reset(
            openperf::message::pop<std::vector<std::string>*>(msg));
        return request;
    }
    case utils::variant_index<api_request, request::statistic::list>():
        return request::statistic::list{};
    case utils::variant_index<api_request, request::statistic::get>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) {
            return request::statistic::get{{.id = std::move(id)}};
        }
        return request::statistic::get{};
    }
    case utils::variant_index<api_request, request::statistic::erase>(): {
        auto id = openperf::message::pop_string(msg);
        if (id.length()) {
            return request::statistic::erase{{.id = std::move(id)}};
        }
        return request::statistic::erase{};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<api_reply, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<api_reply>().index());
    auto idx = openperf::message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<api_reply, reply::ok>():
        return reply::ok{};
    case utils::variant_index<api_reply, reply::info>():
        return openperf::message::pop<reply::info>(msg);
    case utils::variant_index<api_reply, reply::error>(): {
        reply::error reply{};
        reply.data.reset(
            openperf::message::pop<reply::error::error_data*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::generator::item>(): {
        reply::generator::item reply{};
        reply.data.reset(
            openperf::message::pop<reply::generator::item::item_data*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::generator::list>(): {
        reply::generator::list reply{};
        reply.data.reset(openperf::message::pop<
                         std::vector<reply::generator::item::item_data>*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::statistic::item>(): {
        reply::statistic::item reply{};
        reply.data.reset(
            openperf::message::pop<reply::statistic::item::item_data*>(msg));
        return reply;
    }
    case utils::variant_index<api_reply, reply::statistic::list>(): {
        reply::statistic::list reply{};
        reply.data.reset(openperf::message::pop<
                         std::vector<reply::statistic::item::item_data>*>(msg));
        return reply;
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
