#ifndef _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
#define _OP_MEMORY_SWAGGER_CONVERTERS_HPP_

#include <iomanip>

#include "models/tvlp_config.hpp"
#include "swagger/v1/model/TvlpConfiguration.h"

namespace openperf::tvlp::api {

using config_t = tvlp::model::tvlp_configuration_t;
using profile_t = tvlp::model::tvlp_profile_t;
using profile_entry_t = tvlp::model::tvlp_profile_entry_t;

namespace swagger = ::swagger::v1::model;

static config_t from_swagger(const swagger::TvlpConfiguration& m)
{
    auto config = config_t{};
    config.id(m.getId());
    profile_t profile;
    if (m.getProfile()->blockIsSet()) {
        profile.block = std::vector<profile_entry_t>();
        for (auto block_entry : m.getProfile()->getBlock()->getSeries()) {
            profile.block.value().push_back(profile_entry_t{
                .length = static_cast<uint64_t>(block_entry->getLength())});
        }
    }
    if (m.getProfile()->memoryIsSet()) {
        profile.memory = std::vector<profile_entry_t>();
        for (auto memory_entry : m.getProfile()->getMemory()->getSeries()) {
            profile.memory.value().push_back(profile_entry_t{
                .length = static_cast<uint64_t>(memory_entry->getLength())});
        }
    }
    if (m.getProfile()->cpuIsSet()) {
        profile.cpu = std::vector<profile_entry_t>();
        for (auto cpu_entry : m.getProfile()->getCpu()->getSeries()) {
            profile.cpu.value().push_back(profile_entry_t{
                .length = static_cast<uint64_t>(cpu_entry->getLength())});
        }
    }
    if (m.getProfile()->packetIsSet()) {
        profile.packet = std::vector<profile_entry_t>();
        for (auto packet_entry : m.getProfile()->getPacket()->getSeries()) {
            profile.packet.value().push_back(profile_entry_t{
                .length = static_cast<uint64_t>(packet_entry->getLength())});
        }
    }
    config.profile(profile);
    return config;
}

static swagger::TvlpConfiguration to_swagger(const config_t& config)
{
    swagger::TvlpConfiguration model;
    model.setId(config.id());
    model.setTime(std::make_shared<swagger::TvlpConfiguration_time>());
    model.getTime()->setLength(config.total_length());
    if (config.state() == config_t::RUNNING
        || config.state() == config_t::COUNTDOWN)
        model.getTime()->setStart(config.start_time());
    if (config.state() == config_t::RUNNING)
        model.getTime()->setOffset(config.current_offset());
    if (config.state() == config_t::ERROR) model.setError(config.error());

    model.setProfile(std::make_shared<swagger::TvlpProfile>());
    if (config.profile().block) {
        auto block = std::make_shared<swagger::TvlpProfile_block>();
        for (auto block_entry : config.profile().block.value()) {
            auto entry = std::make_shared<swagger::TvlpProfile_block_series>();
            entry->setLength(block_entry.length);
            block->getSeries().push_back(entry);
        }
        model.getProfile()->setBlock(block);
    }

    return model;
}

} // namespace openperf::tvlp::api

#endif // _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
