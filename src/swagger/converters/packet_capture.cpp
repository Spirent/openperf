#include "packet_capture.hpp"

#include "swagger/v1/model/PacketCapture.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, PacketCapture& capture)
{
    /*
     * We can't call capture's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        capture.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        capture.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        capture.setActive(j["active"]);
    }

    if (j.find("direction") != j.end() && !j["direction"].is_null()) {
        capture.setDirection(j["direction"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketCaptureConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        capture.setConfig(config);
    }
}

} // namespace swagger::v1::model