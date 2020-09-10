#include "api.hpp"

#include "framework/message/serialized_message.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"

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
                 [&](request::generator::create& create) -> bool {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<request::generator::create>(
                             std::move(create)));
                 },
                 [&](request::generator::start& start) -> bool {
                     return openperf::message::push(serialized, start.id)
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(start.dynamic_results)));
                 },
                 [&](request::generator::bulk::create& create) -> bool {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<request::generator::bulk::create>(
                             std::move(create)));
                 },
                 [&](request::generator::bulk::start& start) -> bool {
                     return openperf::message::push(
                                serialized,
                                std::make_unique<decltype(start.ids)>(
                                    std::move(start.ids)))
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::configuration>(
                                    std::move(start.dynamic_results)));
                 },
                 [&](request::generator::bulk::id_list& list) -> bool {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<request::generator::bulk::id_list>(
                             std::move(list)));
                 },
                 [&](const id_message& msg) -> bool {
                     return openperf::message::push(serialized, msg.id);
                 },
                 [&](const message&) -> bool { return false; }),
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
                 [&](reply::generator::list& list) -> bool {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::generator::list>(
                             std::move(list)));
                 },
                 [&](reply::generator::item& item) -> bool {
                     return openperf::message::push(serialized, item.id)
                            || openperf::message::push(serialized,
                                                       item.is_running)
                            || openperf::message::push(serialized, item.config)
                            || openperf::message::push(
                                serialized, item.init_percent_complete);
                 },
                 [&](reply::statistic::list& list) -> bool {
                     return openperf::message::push(
                         serialized,
                         std::make_unique<reply::statistic::list>(
                             std::move(list)));
                 },
                 [&](reply::statistic::item& item) -> bool {
                     return openperf::message::push(serialized, item.id)
                            || openperf::message::push(serialized,
                                                       item.generator_id)
                            || openperf::message::push(serialized, item.stat)
                            || openperf::message::push(
                                serialized,
                                std::make_unique<dynamic::results>(
                                    std::move(item.dynamic_results)));
                 },
                 [&](const reply::info& info) -> bool {
                     return openperf::message::push(serialized, info);
                 },
                 [&](reply::error& error) -> bool {
                     return openperf::message::push(serialized, error.type)
                            || openperf::message::push(serialized, error.value)
                            || openperf::message::push(serialized,
                                                       error.message);
                 },
                 [&](const message&) -> bool { return false; }),
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
        return request::generator::get{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::generator::create>(): {
        return std::move(*std::unique_ptr<request::generator::create>(
            openperf::message::pop<request::generator::create*>(msg)));
    }
    case utils::variant_index<api_request, request::generator::erase>(): {
        return request::generator::erase{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request,
                              request::generator::bulk::create>(): {
        return std::move(
            *openperf::message::pop<request::generator::bulk::create*>(msg));
    }
    case utils::variant_index<api_request, request::generator::bulk::erase>(): {
        return std::move(
            *openperf::message::pop<request::generator::bulk::erase*>(msg));
    }
    case utils::variant_index<api_request, request::generator::start>(): {
        request::generator::start request{};
        request.id = openperf::message::pop_string(msg);
        request.dynamic_results =
            std::move(*std::unique_ptr<dynamic::configuration>(
                openperf::message::pop<dynamic::configuration*>(msg)));
        return request;
    }
    case utils::variant_index<api_request, request::generator::stop>(): {
        return request::generator::stop{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::generator::bulk::start>(): {
        request::generator::bulk::start request{};
        request.ids = std::move(*std::unique_ptr<decltype(request.ids)>(
            openperf::message::pop<decltype(request.ids)*>(msg)));
        request.dynamic_results =
            std::move(*std::unique_ptr<dynamic::configuration>(
                openperf::message::pop<dynamic::configuration*>(msg)));
        return request;
    }
    case utils::variant_index<api_request, request::generator::bulk::stop>(): {
        return std::move(
            *openperf::message::pop<request::generator::bulk::stop*>(msg));
    }
    case utils::variant_index<api_request, request::statistic::list>():
        return request::statistic::list{};
    case utils::variant_index<api_request, request::statistic::get>(): {
        return request::statistic::get{
            {.id = openperf::message::pop_string(msg)}};
    }
    case utils::variant_index<api_request, request::statistic::erase>(): {
        return request::statistic::erase{
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
    case utils::variant_index<api_reply, reply::ok>():
        return reply::ok{};
    case utils::variant_index<api_reply, reply::info>():
        return openperf::message::pop<reply::info>(msg);
    case utils::variant_index<api_reply, reply::error>(): {
        reply::error error{};
        error.type = openperf::message::pop<reply::error::type_t>(msg);
        error.value = openperf::message::pop<int>(msg);
        error.message = openperf::message::pop_string(msg);
        return error;
    }
    case utils::variant_index<api_reply, reply::generator::item>(): {
        reply::generator::item item{};
        item.id = openperf::message::pop_string(msg);
        item.is_running = openperf::message::pop<bool>(msg);
        item.config = openperf::message::pop<decltype(item.config)>(msg);
        item.init_percent_complete = openperf::message::pop<int32_t>(msg);
        return item;
    }
    case utils::variant_index<api_reply, reply::generator::list>(): {
        return std::move(*std::unique_ptr<reply::generator::list>(
            openperf::message::pop<reply::generator::list*>(msg)));
    }
    case utils::variant_index<api_reply, reply::statistic::item>(): {
        reply::statistic::item reply{};
        reply.id = openperf::message::pop_string(msg);
        reply.generator_id = openperf::message::pop_string(msg);
        reply.stat = openperf::message::pop<internal::memory_stat>(msg);
        reply.dynamic_results = std::move(*std::unique_ptr<dynamic::results>(
            openperf::message::pop<dynamic::results*>(msg)));
        return reply;
    }
    case utils::variant_index<api_reply, reply::statistic::list>(): {
        return std::move(*std::unique_ptr<reply::statistic::list>(
            openperf::message::pop<reply::statistic::list*>(msg)));
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
