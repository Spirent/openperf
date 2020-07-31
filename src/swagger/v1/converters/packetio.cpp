#include "packetio.hpp"

#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Port.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, Interface& interface)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        interface.setId(j["id"]);
    } else {
        interface.setId(openperf::packetio::empty_id_string);
    }

    interface.setPortId(j.at("port_id"));

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<Interface_config>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        interface.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<InterfaceStats>();
        stats->fromJson(const_cast<nlohmann::json&>(j["stats"]));
        interface.setStats(stats);
    }
}

void from_json(const nlohmann::json& j, Port& port)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        port.setId(j["id"]);
    } else {
        port.setId(openperf::packetio::empty_id_string);
    }

    port.setKind(j.at("kind"));

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PortConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        port.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<PortStats>();
        stats->fromJson(const_cast<nlohmann::json&>(j["stats"]));
        port.setStats(stats);
    }

    if (j.find("status") != j.end() && !j["status"].is_null()) {
        auto status = std::make_shared<PortStatus>();
        status->fromJson(const_cast<nlohmann::json&>(j["status"]));
        port.setStatus(status);
    }
}

} // namespace swagger::v1::model
