
#include "block.hpp"

#include "swagger/v1/model/BlockGenerator.h"
#include "swagger/v1/model/BulkCreateBlockFilesRequest.h"
#include "swagger/v1/model/BulkDeleteBlockFilesRequest.h"
#include "swagger/v1/model/BulkCreateBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopBlockGeneratorsRequest.h"
#include "swagger/v1/model/BlockFile.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, BlockFile& file)
{
    file.setFileSize(j.at("file_size"));
    file.setPath(j.at("path"));

    if (j.find("id") != j.end()) file.setId(j.at("id"));

    if (j.find("init_percent_complete") != j.end())
        file.setInitPercentComplete(j.at("init_percent_complete"));

    if (j.find("state") != j.end()) file.setState(j.at("state"));
}

void from_json(const nlohmann::json& j, BulkCreateBlockFilesRequest& request)
{
    request.getItems().clear();
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<BlockFile> newItem(new BlockFile());
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j, BulkDeleteBlockFilesRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BlockGenerator& generator)
{
    generator.setResourceId(j.at("resource_id"));

    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    auto gc = BlockGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<BlockGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j,
               BulkCreateBlockGeneratorsRequest& request)
{
    request.getItems().clear();
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<BlockGenerator> newItem(new BlockGenerator());
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j,
               BulkDeleteBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStartBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BulkStopBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model