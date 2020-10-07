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

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::CpuGeneratorCoreConfig& config)
{
    auto errors = std::vector<std::string>();

    if (config.getUtilization() < 0 || 100 < config.getUtilization()) {
        errors.emplace_back("Utilization value '"
                            + std::to_string(config.getUtilization())
                            + "' is not valid");
    }

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::CpuGeneratorConfig& config)
{
    auto errors = std::vector<std::string>();

    auto method = config.getMethod();
    if (!config.methodIsSet() || method.empty()) {
        errors.emplace_back("Parameter 'method' is required.");
    } else if (method == "system") {
        auto utilizaton = config.getSystem()->getUtilization();
        if (utilizaton <= 0.0) {
            errors.emplace_back("System utilization value '"
                                + std::to_string(utilizaton)
                                + "' is not valid");
        }
    } else if (method == "cores") {
        auto cores =
            const_cast<swagger::CpuGeneratorConfig&>(config).getCores();

        // Check the cores configuration
        if (cores.empty()) {
            errors.emplace_back("Empty cores array is not allowed");
        }

        for (auto& core : cores) {
            if (auto result = is_valid(*core); !result) {
                std::copy(result.error().begin(),
                          result.error().end(),
                          std::back_inserter(errors));
            }
        }
    } else {
        errors.emplace_back("Method '" + method + "' is not available.");
    }

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

model::generator from_swagger(const swagger::CpuGenerator& generator)
{
    generator_config config;

    auto swagger_cpu_config = generator.getConfig();
    auto method = swagger_cpu_config->getMethod();
    if (method == "system") {
        config.utilization = swagger_cpu_config->getSystem()->getUtilization();
    } else if (method == "cores") {
        auto& configuration = swagger_cpu_config->getCores();
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
    }

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
    request_cpu_generator_bulk_del request{};
    for (auto& id : p_request.getIds()) {
        request.ids.push_back(std::make_unique<std::string>(id));
    }
    return request;
}

std::shared_ptr<swagger::CpuGenerator> to_swagger(const model::generator& model)
{
    auto cpu_config = std::make_shared<swagger::CpuGeneratorConfig>();

    if (auto utilization = model.config().utilization) {
        cpu_config->setMethod("system");

        auto system = std::make_shared<swagger::CpuGeneratorConfigSystem>();
        system->setUtilization(utilization.value());
        cpu_config->setSystem(system);
    } else {
        cpu_config->setMethod("cores");
    }

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
    gen->setTimestampLast(to_rfc3339(result.timestamp().time_since_epoch()));
    gen->setTimestampFirst(
        to_rfc3339(result.start_timestamp().time_since_epoch()));
    gen->setStats(cpu_stats);
    gen->setDynamicResults(std::make_shared<swagger::DynamicResults>(
        dynamic::to_swagger(result.dynamic_results())));

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
