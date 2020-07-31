#include "api_converters.hpp"

#include <iomanip>
#include <sstream>

namespace openperf::cpu::api {

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return os.str();
}

model::generator from_swagger(const swagger::CpuGenerator& generator)
{
    generator_config config;

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
from_swagger(swagger::BulkCreateCpuGeneratorsRequest& p_request)
{
    request_cpu_generator_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.generators.emplace_back(
            std::make_unique<model::generator>(from_swagger(*item)));
    return request;
}

request_cpu_generator_bulk_del
from_swagger(swagger::BulkDeleteCpuGeneratorsRequest& p_request)
{
    request_cpu_generator_bulk_del request{
        std::make_unique<std::vector<std::string>>()};
    for (auto& id : p_request.getIds()) request.ids->push_back(id);
    return request;
}

std::shared_ptr<swagger::CpuGenerator> to_swagger(const model::generator& model)
{
    auto cpu_config = std::make_shared<swagger::CpuGeneratorConfig>();

    auto cores = model.config().cores;
    std::transform(
        cores.begin(),
        cores.end(),
        std::back_inserter(cpu_config->getCores()),
        [](const auto& core_config) {
            auto conf = std::make_shared<swagger::CpuGeneratorCoreConfig>();
            conf->setUtilization(core_config.utilization);

            std::transform(
                core_config.targets.begin(),
                core_config.targets.end(),
                std::back_inserter(conf->getTargets()),
                [](const auto& t) {
                    auto target = std::make_shared<
                        swagger::CpuGeneratorCoreConfig_targets>();
                    target->setDataType(std::string(to_string(t.data_type)));
                    target->setInstructionSet(std::string(to_string(t.set)));
                    target->setWeight(t.weight);
                    return target;
                });

            return conf;
        });

    auto gen = std::make_shared<swagger::CpuGenerator>();
    gen->setId(model.id());
    gen->setRunning(model.running());
    gen->setConfig(cpu_config);
    return gen;
}

std::shared_ptr<swagger::CpuGeneratorResult>
to_swagger(const model::generator_result& result)
{
    auto stats = result.stats();
    auto cpu_stats = std::make_shared<swagger::CpuGeneratorStats>();
    cpu_stats->setUtilization(stats.utilization.count());
    cpu_stats->setAvailable(stats.available.count());
    cpu_stats->setSystem(stats.system.count());
    cpu_stats->setUser(stats.user.count());
    cpu_stats->setSteal(stats.steal.count());
    cpu_stats->setError(stats.error.count());

    auto& cores = stats.cores;
    std::transform(
        cores.begin(),
        cores.end(),
        std::back_inserter(cpu_stats->getCores()),
        [](const auto& c_stats) {
            auto stats = std::make_shared<swagger::CpuGeneratorCoreStats>();
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
                        std::make_shared<swagger::CpuGeneratorTargetStats>();
                    target->setOperations(t_stats.operations);
                    return target;
                });

            return stats;
        });

    auto gen = std::make_shared<swagger::CpuGeneratorResult>();
    gen->setId(result.id());
    gen->setGeneratorId(result.generator_id());
    gen->setActive(result.active());
    gen->setTimestamp(to_rfc3339(result.timestamp().time_since_epoch()));
    gen->setStats(cpu_stats);
    return gen;
}

std::shared_ptr<swagger::CpuInfoResult> to_swagger(const cpu_info_t& info)
{
    auto cpu_info = std::make_shared<swagger::CpuInfoResult>();
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
            auto new_item = std::make_shared<CpuGenerator>();
            from_json(item, *new_item);
            request.getItems().push_back(new_item);
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
