#include <sstream>
#include <iomanip>
#include <zmq.h>

#include "cpu/api.hpp"
#include "message/serialized_message.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"
#include "swagger/v1/model/BulkCreateCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"
#include "swagger/v1/model/CpuInfoResult.h"

namespace openperf::cpu::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_cpu_generator_list&) {
                     return message::push(serialized, 0);
                 },
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
                     return message::push(serialized, request.ids);
                 },
                 [&](const request_cpu_generator_start& cpu_generator) {
                     return message::push(serialized, cpu_generator.id);
                 },
                 [&](const request_cpu_generator_stop& cpu_generator) {
                     return message::push(serialized, cpu_generator.id);
                 },
                 [&](request_cpu_generator_bulk_start& request) {
                     return message::push(serialized, request.ids);
                 },
                 [&](request_cpu_generator_bulk_stop& request) {
                     return message::push(serialized, request.ids);
                 },
                 [&](const request_cpu_generator_result_list&) {
                     return message::push(serialized, 0);
                 },
                 [&](const request_cpu_generator_result& result) {
                     return message::push(serialized, result.id);
                 },
                 [&](const request_cpu_generator_result_del& result) {
                     return message::push(serialized, result.id);
                 },
                 [&](const request_cpu_info&) {
                     return message::push(serialized, 0);
                 }),
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
                 [&](const reply_ok&) { return message::push(serialized, 0); },
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
        return request_cpu_generator_bulk_del{
            message::pop_unique_vector<std::string>(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_start>(): {
        return request_cpu_generator_start{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_stop>(): {
        return request_cpu_generator_stop{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_start>(): {
        return request_cpu_generator_bulk_start{
            message::pop_unique_vector<std::string>(msg)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_stop>(): {
        return request_cpu_generator_bulk_stop{
            message::pop_unique_vector<std::string>(msg)};
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

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return os.str();
}

model::generator from_swagger(const CpuGenerator& gen)
{
    model::generator_config config;
    for (const auto& conf : gen.getConfig()->getCores()) {
        model::generator_core_config core_conf{.utilization =
                                                   conf->getUtilization()};

        for (const auto& target : conf->getTargets()) {
            core_conf.targets.push_back(model::generator_target_config{
                .instruction_set =
                    to_instruction_set(target->getInstructionSet()),
                .data_type = to_data_type(target->getDataType()),
                .weight = static_cast<uint>(target->getWeight())});
        }

        config.cores.push_back(core_conf);
    }

    model::generator gen_model;
    gen_model.id(gen.getId());
    gen_model.running(gen.isRunning());
    gen_model.config(config);
    return gen_model;
}

request_cpu_generator_bulk_add
from_swagger(BulkCreateCpuGeneratorsRequest& p_request)
{
    request_cpu_generator_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.generators.emplace_back(
            std::make_unique<model::generator>(from_swagger(*item)));
    return request;
}

request_cpu_generator_bulk_del
from_swagger(BulkDeleteCpuGeneratorsRequest& p_request)
{
    request_cpu_generator_bulk_del request{};
    for (auto& id : p_request.getIds())
        request.ids.push_back(std::make_unique<std::string>(id));
    return request;
}

std::shared_ptr<CpuGenerator> to_swagger(const model::generator& model)
{
    auto cpu_config = std::make_shared<CpuGeneratorConfig>();

    auto cores = model.config().cores;
    std::transform(
        cores.begin(),
        cores.end(),
        std::back_inserter(cpu_config->getCores()),
        [](const auto& core_config) {
            auto conf = std::make_shared<CpuGeneratorCoreConfig>();
            conf->setUtilization(core_config.utilization);

            std::transform(
                core_config.targets.begin(),
                core_config.targets.end(),
                std::back_inserter(conf->getTargets()),
                [](const auto& t) {
                    auto target =
                        std::make_shared<CpuGeneratorCoreConfig_targets>();
                    target->setDataType(to_string(t.data_type));
                    target->setInstructionSet(to_string(t.instruction_set));
                    target->setWeight(t.weight);
                    return target;
                });

            return conf;
        });

    auto gen = std::make_shared<CpuGenerator>();
    gen->setId(model.id());
    gen->setRunning(model.running());
    gen->setConfig(cpu_config);
    return gen;
}

std::shared_ptr<CpuGeneratorResult>
to_swagger(const model::generator_result& result)
{
    auto stats = result.stats();
    auto cpu_stats = std::make_shared<CpuGeneratorStats>();
    cpu_stats->setUtilization(stats.utilization.count());
    cpu_stats->setAvailable(stats.available.count());
    cpu_stats->setSystem(stats.system.count());
    cpu_stats->setUser(stats.user.count());
    cpu_stats->setSteal(stats.steal.count());
    cpu_stats->setError(stats.error.count());

    for (const auto& c_stats : stats.cores) {
        auto core_stats = std::make_shared<CpuGeneratorCoreStats>();
        for (const auto& c_target : c_stats.targets) {
            auto target = std::make_shared<CpuGeneratorTargetStats>();
            target->setOperations(c_target.operations);
            core_stats->getTargets().push_back(target);
        }
        cpu_stats->getCores().push_back(core_stats);

        core_stats->setAvailable(c_stats.available.count());
        core_stats->setError(c_stats.error.count());
        core_stats->setSteal(c_stats.steal.count());
        core_stats->setSystem(c_stats.system.count());
        core_stats->setUser(c_stats.user.count());
        core_stats->setUtilization(c_stats.utilization.count());
    }

    auto gen = std::make_shared<CpuGeneratorResult>();
    gen->setId(result.id());
    gen->setGeneratorId(result.generator_id());
    gen->setActive(result.active());
    gen->setTimestamp(to_rfc3339(result.timestamp().time_since_epoch()));
    gen->setStats(cpu_stats);
    return gen;
}

std::shared_ptr<CpuInfoResult> to_swagger(const cpu_info_t& info)
{
    auto cpu_info = std::make_shared<CpuInfoResult>();
    cpu_info->setArchitecture(info.architecture);
    cpu_info->setCacheLineSize(info.cache_line_size);
    cpu_info->setCores(info.cores);
    return cpu_info;
}

} // namespace openperf::cpu::api

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, CpuGenerator& generator)
{
    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    auto gc = CpuGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<CpuGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j, BulkCreateCpuGeneratorsRequest& request)
{
    request.getItems().clear();
    nlohmann::json jsonArray;
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<CpuGenerator> newItem(new CpuGenerator());
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j, BulkDeleteCpuGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BulkStartCpuGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BulkStopCpuGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model
