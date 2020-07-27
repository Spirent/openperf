#include <sstream>
#include <iomanip>

#include "api.hpp"

#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"

#include "swagger/v1/model/BulkCreateCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"
#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"
#include "swagger/v1/model/CpuInfoResult.h"

namespace openperf::cpu::api {

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
        return error;
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(msg));
    *ptr = value.release();
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

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg,
                         std::vector<std::unique_ptr<T>>& values)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*) * values.size());
        error != 0) {
        return error;
    }

    auto cursor = reinterpret_cast<T**>(zmq_msg_data(msg));
    std::transform(std::begin(values), std::end(values), cursor, [](auto& ptr) {
        return ptr.release();
    });
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

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_cpu_generator_list&) {
                     return zmq_msg_init(&serialized.data);
                 },
                 [&](const request_cpu_generator& cpu_generator) {
                     return zmq_msg_init(&serialized.data,
                                         cpu_generator.id.data(),
                                         cpu_generator.id.length());
                 },
                 [&](request_cpu_generator_add& cpu_generator) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(cpu_generator.source));
                 },
                 [&](const request_cpu_generator_del& cpu_generator) {
                     return zmq_msg_init(&serialized.data,
                                         cpu_generator.id.data(),
                                         cpu_generator.id.length());
                 },
                 [&](request_cpu_generator_bulk_add& request) {
                     return zmq_msg_init(&serialized.data, request.generators);
                 },
                 [&](request_cpu_generator_bulk_del& request) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(request.ids));
                 },
                 [&](const request_cpu_generator_start& cpu_generator) {
                     return zmq_msg_init(&serialized.data,
                                         cpu_generator.id.data(),
                                         cpu_generator.id.length());
                 },
                 [&](const request_cpu_generator_stop& cpu_generator) {
                     return zmq_msg_init(&serialized.data,
                                         cpu_generator.id.data(),
                                         cpu_generator.id.length());
                 },
                 [&](request_cpu_generator_bulk_start& request) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(request.ids));
                 },
                 [&](request_cpu_generator_bulk_stop& request) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(request.ids));
                 },
                 [&](const request_cpu_generator_result_list&) {
                     return zmq_msg_init(&serialized.data);
                 },
                 [&](const request_cpu_generator_result& result) {
                     return zmq_msg_init(&serialized.data,
                                         result.id.data(),
                                         result.id.length());
                 },
                 [&](const request_cpu_generator_result_del& result) {
                     return zmq_msg_init(&serialized.data,
                                         result.id.data(),
                                         result.id.length());
                 },
                 [&](const request_cpu_info&) {
                     return zmq_msg_init(&serialized.data);
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](reply_cpu_generators& reply) {
                     return zmq_msg_init(&serialized.data, reply.generators);
                 },
                 [&](reply_cpu_generator_results& reply) {
                     return zmq_msg_init(&serialized.data, reply.results);
                 },
                 [&](reply_cpu_info& reply) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(reply.info));
                 },
                 [&](const reply_ok&) {
                     return zmq_msg_init(&serialized.data, 0);
                 },
                 [&](reply_error& error) {
                     return zmq_msg_init(&serialized.data,
                                         std::move(error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return serialized;
}

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<request_msg, request_cpu_generator_list>(): {
        return request_cpu_generator_list{};
    }
    case utils::variant_index<request_msg, request_cpu_generator_add>(): {
        auto request = request_cpu_generator_add{};
        request.source.reset(*zmq_msg_data<cpu_generator_t**>(&msg.data));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator{std::move(id)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator_del{std::move(id)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_add>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(cpu_generator_t*);
        auto data = zmq_msg_data<cpu_generator_t**>(&msg.data);

        auto request = request_cpu_generator_bulk_add{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.generators.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_del>(): {
        auto request = request_cpu_generator_bulk_del{};
        request.ids.reset(*zmq_msg_data<std::vector<std::string>**>(&msg.data));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator_start{std::move(id)};
    }
    case utils::variant_index<request_msg, request_cpu_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator_stop{std::move(id)};
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_start>(): {
        auto request = request_cpu_generator_bulk_start{};
        request.ids.reset(*zmq_msg_data<std::vector<std::string>**>(&msg.data));
        return request;
    }
    case utils::variant_index<request_msg, request_cpu_generator_bulk_stop>(): {
        auto request = request_cpu_generator_bulk_stop{};
        request.ids.reset(*zmq_msg_data<std::vector<std::string>**>(&msg.data));
        return request;
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_list>(): {
        return request_cpu_generator_result_list{};
    }
    case utils::variant_index<request_msg, request_cpu_generator_result>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator_result{std::move(id)};
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return request_cpu_generator_result_del{std::move(id)};
    }
    case utils::variant_index<request_msg, request_cpu_info>(): {
        return request_cpu_info{};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<reply_msg, reply_cpu_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(cpu_generator_t*);
        auto data = zmq_msg_data<cpu_generator_t**>(&msg.data);

        auto reply = reply_cpu_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.generators.emplace_back(ptr);
        });
        return reply;
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_results>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(cpu_generator_result_t*);
        auto data = zmq_msg_data<cpu_generator_result_t**>(&msg.data);

        auto reply = reply_cpu_generator_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.results.emplace_back(ptr);
        });
        return reply;
    }
    case utils::variant_index<reply_msg, reply_cpu_info>(): {
        auto reply = reply_cpu_info{};
        reply.info.reset(*zmq_msg_data<cpu_info_t**>(&msg.data));
        return reply;
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return reply_ok{};
    case utils::variant_index<reply_msg, reply_error>():
        auto reply = reply_error{};
        reply.info.reset(*zmq_msg_data<typed_error**>(&msg.data));
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

model::generator from_swagger(const CpuGenerator& generator)
{
    model::generator_config config;

    auto& configuration = generator.getConfig()->getCores();
    std::transform(
        configuration.begin(),
        configuration.end(),
        std::back_inserter(config.cores),
        [](const auto& conf) {
            task_cpu_config task_conf{
                .utilization = conf->getUtilization(),
            };

            auto& targets = conf->getTargets();
            std::transform(
                targets.begin(),
                targets.end(),
                std::back_inserter(task_conf.targets),
                [](const auto& t) {
                    return target_config{
                        .set = to_instruction_set(t->getInstructionSet()),
                        .data_type = to_data_type(t->getDataType()),
                        .weight = static_cast<uint>(t->getWeight()),
                    };
                });

            return task_conf;
        });

    model::generator gen_model;
    gen_model.id(generator.getId());
    gen_model.running(generator.isRunning());
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
    request_cpu_generator_bulk_del request{
        std::make_unique<std::vector<std::string>>()};
    for (auto& id : p_request.getIds()) request.ids->push_back(id);
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
                    target->setDataType(std::string(to_string(t.data_type)));
                    target->setInstructionSet(std::string(to_string(t.set)));
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

    auto& cores = stats.cores;
    std::transform(cores.begin(),
                   cores.end(),
                   std::back_inserter(cpu_stats->getCores()),
                   [](const auto& c_stats) {
                       auto stats = std::make_shared<CpuGeneratorCoreStats>();
                       stats->setAvailable(c_stats.available.count());
                       stats->setError(c_stats.error.count());
                       stats->setSteal(c_stats.steal.count());
                       stats->setSystem(c_stats.system.count());
                       stats->setUser(c_stats.user.count());
                       stats->setUtilization(c_stats.utilization.count());

                       auto& targets = c_stats.targets;
                       std::transform(
                           targets.begin(),
                           targets.end(),
                           std::back_inserter(stats->getTargets()),
                           [](const auto& t_stats) {
                               auto target =
                                   std::make_shared<CpuGeneratorTargetStats>();
                               target->setOperations(t_stats.operations);
                               return target;
                           });

                       return stats;
                   });

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
