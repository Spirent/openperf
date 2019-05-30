#include "packetio/json_transmogrify.h"

using json = nlohmann::json;

namespace swagger {
namespace v1 {
namespace model {

void from_json(const json& j, Interface& interface)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        interface.setId(j["id"]);
    } else {
        interface.setId(icp::packetio::empty_id_string);
    }

    interface.setPortId(j.at("port_id"));

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<Interface_config>();
        config->fromJson(const_cast<json&>(j["config"]));
        interface.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<InterfaceStats>();
        stats->fromJson(const_cast<json&>(j["stats"]));
        interface.setStats(stats);
    }
}

void from_json(const json& j, Port& port)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        port.setId(j["id"]);
    } else {
        port.setId(icp::packetio::empty_id_string);
    }

    port.setKind(j.at("kind"));

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PortConfig>();
        config->fromJson(const_cast<json&>(j["config"]));
        port.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<PortStats>();
        stats->fromJson(const_cast<json&>(j["stats"]));
        port.setStats(stats);
    }

    if (j.find("status") != j.end() && !j["status"].is_null()) {
        auto status = std::make_shared<PortStatus>();
        status->fromJson(const_cast<json&>(j["status"]));
        port.setStatus(status);
    }
}

}
}
}
