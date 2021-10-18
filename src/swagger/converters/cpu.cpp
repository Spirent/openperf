#include "cpu.hpp"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/BulkCreateCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"
#include "swagger/v1/model/CpuGeneratorResult.h"

#include <iostream>

namespace swagger::v1::model {

/* std::string needed by json find/[] functions */
static nlohmann::json* find_key(const nlohmann::json& j, const std::string& key)
{
    auto ptr = j.find(key) != j.end() && !j[key].is_null()
                   ? std::addressof(j[key])
                   : nullptr;
    return (const_cast<nlohmann::json*>(ptr));
}

void from_json(const nlohmann::json& j, CpuGeneratorCoreConfig& core_config)
{
    if (auto jutil = find_key(j, "utilization")) {
        core_config.setUtilization(jutil->get<double>());
    }

    /* Targets might be non-existent */
    if (!find_key(j, "targets")) { return; }

    std::transform(std::begin(j["targets"]),
                   std::end(j["targets"]),
                   std::back_inserter(core_config.getTargets()),
                   [](const auto& j_target) {
                       auto target =
                           std::make_shared<CpuGeneratorCoreConfig_targets>();

                       if (auto jweight = find_key(j_target, "weight")) {
                           target->setWeight(jweight->template get<int>());
                       }

                       if (auto jload = find_key(j_target, "load")) {
                           auto load = std::make_shared<CpuGeneratorCoreLoad>();
                           load->fromJson(*jload);
                           target->setLoad(load);
                       }

                       return (target);
                   });
}

void from_json(const nlohmann::json& j, CpuGeneratorConfig& config)
{
    auto jmethod = find_key(j, "method");
    if (!jmethod) { return; }

    auto method = jmethod->get<std::string>();

    if (method == "system") {
        config.fromJson(const_cast<nlohmann::json&>(j));
        return;
    }

    config.setMethod(method);
    std::transform(std::begin(j["cores"]),
                   std::end(j["cores"]),
                   std::back_inserter(config.getCores()),
                   [](const auto& j_config) {
                       auto config = std::make_shared<CpuGeneratorCoreConfig>();
                       *config =
                           j_config.template get<CpuGeneratorCoreConfig>();
                       return (config);
                   });
}

void from_json(const nlohmann::json& j, CpuGenerator& generator)
{
    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    if (auto jconfig = find_key(j, "config")) {
        auto config = std::make_shared<CpuGeneratorConfig>();
        *config = jconfig->template get<CpuGeneratorConfig>();
        generator.setConfig(config);
    }
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

void from_json(const nlohmann::json& j, CpuGeneratorResult& result)
{
    result.fromJson(const_cast<nlohmann::json&>(j));

    auto stats = std::make_shared<CpuGeneratorStats>();
    stats->fromJson(const_cast<nlohmann::json&>(j).at("stats"));
    result.setStats(stats);

    /*
     * XXX: fromJson doesn't copy the deeply nested load config over,
     * so do it manually.
     */
    auto& cores = stats->getCores();
    auto core_idx = 0;
    std::for_each(std::begin(cores), std::end(cores), [&](auto& core) {
        auto& targets = core->getTargets();
        auto target_idx = 0;
        std::for_each(
            std::begin(targets), std::end(targets), [&](auto& target) {
                auto j_target =
                    j["stats"]["cores"][core_idx]["targets"][target_idx++];
                auto load = std::make_shared<CpuGeneratorCoreLoad>();
                load->fromJson(j_target.at("load"));
                target->setLoad(load);
            });
        core_idx++;
    });
}

} // namespace swagger::v1::model
