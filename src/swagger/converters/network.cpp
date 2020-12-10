#include "memory.hpp"

#include "swagger/v1/model/NetworkGenerator.h"
#include "swagger/v1/model/BulkCreateNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopNetworkGeneratorsRequest.h"
#include "swagger/v1/model/NetworkGeneratorResult.h"
#include "swagger/v1/model/NetworkServer.h"
#include "swagger/v1/model/BulkCreateNetworkServersRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkServersRequest.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, NetworkGenerator& generator)
{
    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    auto gc = NetworkGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<NetworkGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j,
               BulkCreateNetworkGeneratorsRequest& request)
{
    request.getItems().clear();
    nlohmann::json jsonArray;
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) { continue; }
        std::shared_ptr<NetworkGenerator> newItem =
            std::make_shared<NetworkGenerator>();
        from_json(item, *newItem);
        request.getItems().push_back(newItem);
    }
}

void from_json(const nlohmann::json& j,
               BulkDeleteNetworkGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStartNetworkGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStopNetworkGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, NetworkGeneratorResult& result)
{
    result.fromJson(const_cast<nlohmann::json&>(j));

    auto write_stats = std::make_shared<NetworkGeneratorStats>();
    write_stats->fromJson(const_cast<nlohmann::json&>(j).at("write"));
    result.setWrite(write_stats);
    auto read_stats = std::make_shared<NetworkGeneratorStats>();
    read_stats->fromJson(const_cast<nlohmann::json&>(j).at("read"));
    result.setRead(read_stats);
    auto connection_stats = std::make_shared<NetworkGeneratorConnectionStats>();
    connection_stats->fromJson(const_cast<nlohmann::json&>(j).at("connection"));
    result.setConnection(connection_stats);
}

void from_json(const nlohmann::json& j, NetworkServer& server)
{
    if (j.find("id") != j.end()) { server.setId(j.at("id")); }
    if (j.find("interface") != j.end()) {
        server.setInterface(j.at("interface"));
    }
    if (j.find("address_family") != j.end()) {
        server.setAddressFamily(j.at("address_family"));
    }
    server.setPort(j.at("port"));
    server.setProtocol(j.at("protocol"));
}

void from_json(const nlohmann::json& j,
               BulkCreateNetworkServersRequest& request)
{
    request.getItems().clear();
    nlohmann::json jsonArray;
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) { continue; }
        std::shared_ptr<NetworkServer> newItem =
            std::make_shared<NetworkServer>();
        from_json(item, *newItem);
        request.getItems().push_back(newItem);
    }
}

void from_json(const nlohmann::json& j,
               BulkDeleteNetworkServersRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model