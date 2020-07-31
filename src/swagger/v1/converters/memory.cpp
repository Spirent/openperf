#include "memory.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/BulkCreateMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"

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