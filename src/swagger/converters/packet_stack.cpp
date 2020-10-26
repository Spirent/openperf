#include "swagger/converters/packet_stack.hpp"
#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Stack.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, Interface& interface)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        interface.setId(j["id"]);
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

} // namespace swagger::v1::model
