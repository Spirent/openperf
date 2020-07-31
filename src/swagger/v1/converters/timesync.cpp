#include "timesync.hpp"

#include "swagger/v1/model/TimeSource.h"
#include "core/op_core.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, TimeSource& ts)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        ts.setId(j["id"]);
    } else {
        ts.setId(openperf::core::to_string(openperf::core::uuid::random()));
    }

    if (j.find("kind") != j.end() && !j["kind"].is_null()) {
        ts.setKind(j["kind"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<TimeSourceConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        ts.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<TimeSourceStats>();
        stats->fromJson(const_cast<nlohmann::json&>(j["stats"]));
        ts.setStats(stats);
    }
}

} // namespace swagger::v1::model