#include "packetio/json_transmogrify.h"

using json = nlohmann::json;

namespace swagger {
namespace v1 {
namespace model {

void from_json(const json &j, Interface& interface)
{
    if (j.find("id") != j.end()) {
        interface.fromJson(const_cast<json&>(j));
    }

    if (j.find("config") != j.end()) {
        auto config = std::make_shared<Interface_config>();
        config->fromJson(const_cast<json&>(j.at("config")));
        interface.setConfig(config);
    }

    if (j.find("stats") != j.end()) {
        auto stats = std::make_shared<InterfaceStats>();
        stats->fromJson(const_cast<json&>(j.at("stats")));
        interface.setStats(stats);
    }
}

void from_json(const json& j, Port& port)
{
    if (j.find("id") != j.end()
        && j.find("kind") != j.end()) {
        port.fromJson(const_cast<json&>(j));
    }

    if (j.find("driver") != j.end()) {
        auto driver = std::make_shared<PortDriver>();
        driver->fromJson(const_cast<json&>(j.at("driver")));
        port.setDriver(driver);
    }

    if (j.find("config") != j.end()) {
        auto config = std::make_shared<PortConfig>();
        config->fromJson(const_cast<json&>(j.at("config")));
        port.setConfig(config);
    }

    if (j.find("status") != j.end()) {
        auto status = std::make_shared<PortStatus>();
        status->fromJson(const_cast<json&>(j.at("status")));
        port.setStatus(status);
    }

    if (j.find("stats") != j.end()) {
        auto stats = std::make_shared<PortStats>();
        stats->fromJson(const_cast<json&>(j.at("stats")));
        port.setStats(stats);
    }
}

}
}
}
