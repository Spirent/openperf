#include "packet_analyzer.hpp"

#include "swagger/v1/model/PacketAnalyzer.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, PacketAnalyzer& analyzer)
{
    /*
     * We can't call analyzer's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        analyzer.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        analyzer.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        analyzer.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketAnalyzerConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        analyzer.setConfig(config);
    }
}

} // namespace swagger::v1::model