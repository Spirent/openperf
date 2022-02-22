#include "zmq.h"

#include "core/op_core.h"
#include "cpu/api.hpp"
#include "cpu/arg_parser.hpp"
#include "dynamic/validator.tcc"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuInfoResult.h"
#include "swagger/v1/model/DynamicResultsConfig.h"

namespace openperf::cpu::api {

struct cpu_dynamic_validator : public dynamic::validator<cpu_dynamic_validator>
{
    bool static is_valid_stat(std::string_view name);
};

static bool is_valid_name_stat(std::string_view name)
{
    if (name == "timestamp" || name == "available" || name == "utilization"
        || name == "target" || name == "system" || name == "user"
        || name == "steal" || name == "error") {
        return (true);
    }

    return (false);
}

bool cpu_dynamic_validator::is_valid_stat(std::string_view name)
{
    if (is_valid_name_stat(name)) { return (true); }

    auto core_idx = 0U;
    auto instr = std::string(16, '\0');
    auto data = std::string(16, '\0');
    auto op = std::string(10, '\0'); /* "operations".size() == 10 */

    if (sscanf(std::string(name).c_str(),
               "cores[%d].targets[%15[^,],%15[^]]].%10s",
               &core_idx,
               instr.data(),
               data.data(),
               op.data())
            == 4
        && to_instruction_type(instr.c_str()) != instruction_type::none
        && to_data_type(data.c_str()) != data_type::none
        && op == "operations") {
        return (true);
    }

    auto stat = std::string(32, '\0');
    if (sscanf(
            std::string(name).c_str(), "cores[%d].%31s", &core_idx, stat.data())
            == 2
        && is_valid_name_stat(stat.c_str())) {
        return (true);
    }

    return (false);
}

static void is_valid(const swagger::v1::model::CpuGeneratorCoreConfig& config,
                     std::vector<std::string>& errors)
{
    if (config.getUtilization() < 0 || 100 < config.getUtilization()) {
        errors.emplace_back("Utilization value '"
                            + std::to_string(config.getUtilization())
                            + "' is not valid.");
    }

    const auto& targets =
        const_cast<swagger::v1::model::CpuGeneratorCoreConfig&>(config)
            .getTargets();
    if (config.getUtilization() > 0 && targets.empty()) {
        errors.emplace_back("Targets must be specified for non-zero loads.");
    }
    std::for_each(
        std::begin(targets), std::end(targets), [&](const auto& target) {
            auto instr = target->getInstructionSet();
            if (instr.empty()) {
                errors.emplace_back(
                    "Instruction set must be specified for each target.");
            }
            if (to_instruction_type(instr) == instruction_type::none) {
                errors.emplace_back("Instruction set, " + instr
                                    + ", is not valid.");
            }

            auto data = target->getDataType();
            if (data.empty()) {
                errors.emplace_back(
                    "Data type must be specified for each target.");
            }
            if (to_data_type(data) == data_type::none) {
                errors.emplace_back("Data type, " + data + ", is not valid.");
            }
        });
}

static void is_valid(const swagger::v1::model::CpuGeneratorConfig& config,
                     std::vector<std::string>& errors)
{
    auto method = config.getMethod();
    if (method == "system") {
        auto utilizaton = config.getSystem()->getUtilization();
        if (utilizaton <= 0.0) {
            errors.emplace_back("System utilization value '"
                                + std::to_string(utilizaton)
                                + "' is not valid");
        }
    } else if (method == "cores") {
        /* XXX: there is no const version of getCores() */
        const auto& cores =
            const_cast<swagger::v1::model::CpuGeneratorConfig&>(config)
                .getCores();
        if (cores.empty()) {
            errors.emplace_back("Empty cores array is not allowed.");
        } else if (op_cpu_count() < cores.size()) {
            errors.emplace_back("Not enough CPU cores available: have "
                                + std::to_string(op_cpu_count()) + ", need "
                                + std::to_string(cores.size()));
        } else if (auto mask = config::core_mask();
                   mask && mask->count() < cores.size()) {
            errors.emplace_back("To many CPU cores for provided mask: have "
                                + std::to_string(mask->count()) + ", need "
                                + std::to_string(cores.size()));
        } else {
            for (const auto& core : cores) { is_valid(*core, errors); }
        }
    } else {
        errors.emplace_back("Method '" + method + "' is not available.");
    }
}

bool is_valid(const cpu_generator_type& generator,
              std::vector<std::string>& errors)
{
    auto config = generator.getConfig();
    if (!config) {
        errors.emplace_back("Generator configuration is required.");
        return (false);
    }

    is_valid(*config, errors);

    return (errors.empty());
}

bool is_valid(const swagger::v1::model::DynamicResultsConfig& config,
              std::vector<std::string>& errors)
{
    return (cpu_dynamic_validator{}(config, errors));
}

cpu_info_ptr get_cpu_info()
{
    auto info = std::make_unique<swagger::v1::model::CpuInfoResult>();

    info->setCores(op_cpu_count());
    info->setCacheLineSize(op_cpu_l1_cache_line_size());
    info->setArchitecture(std::string(op_cpu_architecture()));

    return (info);
}

const char* to_string(const api::reply_error& error)
{
    switch (error.info.type) {
    case error_type::NOT_FOUND:
        return ("");
    case error_type::ZMQ_ERROR:
        return (zmq_strerror(error.info.value));
    default:
        return (strerror(error.info.value));
    }
}

} // namespace openperf::cpu::api

template struct openperf::dynamic::validator<
    openperf::cpu::api::cpu_dynamic_validator>;
