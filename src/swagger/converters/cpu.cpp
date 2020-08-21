#include "cpu.hpp"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/BulkCreateCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"
#include "swagger/v1/model/CpuGeneratorResult.h"

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

void from_json(const nlohmann::json& j, CpuGeneratorResult& result)
{
    result.fromJson(const_cast<nlohmann::json&>(j));

    auto stats = std::make_shared<CpuGeneratorStats>();
    stats->fromJson(const_cast<nlohmann::json&>(j).at("stats"));
    result.setStats(stats);
}

} // namespace swagger::v1::model