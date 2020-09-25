#include "packet_generator.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"

namespace swagger::v1::model {

/* std::string needed by json find/[] functions */
nlohmann::json* find_key(const nlohmann::json& j, const std::string& key)
{
    auto ptr = j.find(key) != j.end() && !j[key].is_null()
                   ? std::addressof(j[key])
                   : nullptr;
    return (const_cast<nlohmann::json*>(ptr));
}

void from_json(const nlohmann::json& j, TrafficLoad& load)
{
    load.fromJson(const_cast<nlohmann::json&>(j));

    if (auto jrate = find_key(j, "rate")) {
        auto rate = std::make_shared<TrafficLoad_rate>();
        rate->fromJson(*jrate);
        load.setRate(rate);
    }
}

void from_json(const nlohmann::json& j, TrafficDefinition& definition)
{
    definition.fromJson(const_cast<nlohmann::json&>(j));

    if (auto jlength = find_key(j, "length")) {
        auto length = std::make_shared<TrafficLength>();
        length->fromJson(*jlength);
        definition.setLength(length);
    }

    if (auto jpacket = find_key(j, "packet")) {
        auto packet = std::make_shared<TrafficPacketTemplate>();
        packet->fromJson(*jpacket);
        definition.setPacket(packet);
    }
}

void from_json(const nlohmann::json& j, PacketGeneratorConfig& config)
{
    if (auto jduration = find_key(j, "duration")) {
        auto duration = std::make_shared<TrafficDuration>();
        duration->fromJson(*jduration);
        config.setDuration(duration);
    }

    if (auto jload = find_key(j, "load")) {
        auto load = std::make_shared<TrafficLoad>();
        *load = jload->get<TrafficLoad>();
        config.setLoad(load);
    }

    if (auto jorder = find_key(j, "order")) {
        config.setOrder(jorder->get<std::string>());
    }

    if (auto jcounters = find_key(j, "protocol_counters")) {
        std::transform(std::begin(*jcounters),
                       std::end(*jcounters),
                       std::back_inserter(config.getProtocolCounters()),
                       [](const auto& j_counter) {
                           return (j_counter.template get<std::string>());
                       });
    }

    std::transform(std::begin(j["traffic"]),
                   std::end(j["traffic"]),
                   std::back_inserter(config.getTraffic()),
                   [](const auto& j_def) {
                       auto def = std::make_shared<TrafficDefinition>();
                       *def = j_def.template get<TrafficDefinition>();
                       return (def);
                   });
}

void from_json(const nlohmann::json& j, PacketGenerator& generator)
{
    /*
     * We can't call generator's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        generator.setId(j["id"]);
    }

    if (j.find("target_id") != j.end() && !j["target_id"].is_null()) {
        generator.setTargetId(j["target_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        generator.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketGeneratorConfig>();
        *config = j["config"].get<PacketGeneratorConfig>();
        generator.setConfig(config);
    }
}

void from_json(const nlohmann::json& j, PacketGeneratorResult& result)
{
    result.fromJson(const_cast<nlohmann::json&>(j));

    auto flow_counters = std::make_shared<PacketGeneratorFlowCounters>();
    flow_counters->fromJson(const_cast<nlohmann::json&>(j).at("flow_counters"));
    result.setFlowCounters(flow_counters);
    auto protocol_counters =
        std::make_shared<PacketGeneratorProtocolCounters>();
    protocol_counters->fromJson(
        const_cast<nlohmann::json&>(j).at("protocol_counters"));
    result.setProtocolCounters(protocol_counters);
}

} // namespace swagger::v1::model