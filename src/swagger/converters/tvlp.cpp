
#include "tvlp.hpp"

#include "packet_generator.hpp"
#include "swagger/v1/model/TvlpConfiguration.h"

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, TvlpProfile_cpu_series& cpu_series)
{
    auto val = const_cast<nlohmann::json&>(j);

    cpu_series.setLength(val.at("length"));

    auto gc = std::make_shared<CpuGeneratorConfig>();
    gc->fromJson(val.at("config"));
    cpu_series.setConfig(gc);
}

void from_json(const nlohmann::json& j, TvlpProfile_cpu& cpu_profile)
{
    auto val = const_cast<nlohmann::json&>(j);

    cpu_profile.getSeries().clear();
    for (auto& item : val["series"]) {
        auto newItem = std::make_shared<TvlpProfile_cpu_series>(
            item.get<TvlpProfile_cpu_series>());
        cpu_profile.getSeries().push_back(newItem);
    }
}

void from_json(const nlohmann::json& j, TvlpProfile_block_series& block_series)
{
    auto val = const_cast<nlohmann::json&>(j);

    block_series.setResourceId(val.at("resource_id"));
    block_series.setLength(val.at("length"));

    auto gc = std::make_shared<BlockGeneratorConfig>();
    gc->fromJson(val.at("config"));
    block_series.setConfig(gc);
}

void from_json(const nlohmann::json& j, TvlpProfile_block& block_profile)
{
    auto val = const_cast<nlohmann::json&>(j);

    block_profile.getSeries().clear();
    for (auto& item : val["series"]) {
        auto newItem = std::make_shared<TvlpProfile_block_series>(
            item.get<TvlpProfile_block_series>());
        block_profile.getSeries().push_back(newItem);
    }
}

void from_json(const nlohmann::json& j,
               TvlpProfile_memory_series& memory_series)
{
    auto val = const_cast<nlohmann::json&>(j);

    memory_series.setLength(val.at("length"));

    auto gc = std::make_shared<MemoryGeneratorConfig>();
    gc->fromJson(val.at("config"));
    memory_series.setConfig(gc);
}

void from_json(const nlohmann::json& j, TvlpProfile_memory& memory_profile)
{
    auto val = const_cast<nlohmann::json&>(j);

    memory_profile.getSeries().clear();
    if (val.find("series") != val.end()) {
        for (auto& item : val["series"]) {
            auto newItem = std::make_shared<TvlpProfile_memory_series>(
                item.get<TvlpProfile_memory_series>());
            memory_profile.getSeries().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j,
               TvlpProfile_packet_series& packet_series)
{
    auto val = const_cast<nlohmann::json&>(j);

    packet_series.setTargetId(val.at("target_id"));
    packet_series.setLength(val.at("length"));

    auto gc = std::make_shared<PacketGeneratorConfig>();
    *gc = j["config"].get<PacketGeneratorConfig>();
    packet_series.setConfig(gc);
}

void from_json(const nlohmann::json& j, TvlpProfile_packet& packet_profile)
{
    auto val = const_cast<nlohmann::json&>(j);

    packet_profile.getSeries().clear();
    if (val.find("series") != val.end()) {
        for (auto& item : val["series"]) {
            auto newItem = std::make_shared<TvlpProfile_packet_series>(
                item.get<TvlpProfile_packet_series>());
            packet_profile.getSeries().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j, TvlpProfile& profile)
{
    auto val = const_cast<nlohmann::json&>(j);
    if (val.find("memory") != val.end()) {
        if (!val["memory"].is_null()) {
            auto newItem = std::make_shared<TvlpProfile_memory>(
                val.at("memory").get<TvlpProfile_memory>());
            profile.setMemory(newItem);
        }
    }

    if (val.find("block") != val.end()) {
        if (!val["block"].is_null()) {
            auto newItem = std::make_shared<TvlpProfile_block>(
                val.at("block").get<TvlpProfile_block>());
            profile.setBlock(newItem);
        }
    }

    if (val.find("cpu") != val.end()) {
        if (!val["cpu"].is_null()) {
            auto newItem = std::make_shared<TvlpProfile_cpu>(
                val.at("cpu").get<TvlpProfile_cpu>());
            profile.setCpu(newItem);
        }
    }

    if (val.find("packet") != val.end()) {
        if (!val["packet"].is_null()) {
            auto newItem = std::make_shared<TvlpProfile_packet>(
                val.at("packet").get<TvlpProfile_packet>());
            profile.setPacket(newItem);
        }
    }
}

void from_json(const nlohmann::json& j, TvlpConfiguration& generator)
{
    if (j.find("id") != j.end()) generator.setId(j.at("id"));

    if (j.find("time_scale") != j.end())
        generator.setTimeScale(j.at("time_scale"));
    else
        generator.setTimeScale(1.0);

    if (j.find("load_scale") != j.end())
        generator.setLoadScale(j.at("load_scale"));
    else
        generator.setLoadScale(1.0);

    generator.setProfile(
        std::make_shared<TvlpProfile>(j.at("profile").get<TvlpProfile>()));
}

} // namespace swagger::v1::model