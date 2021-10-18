#include "api_converters.hpp"

#include <iomanip>

#include "swagger/converters/block.hpp"
#include "swagger/converters/memory.hpp"
#include "swagger/converters/cpu.hpp"
#include "swagger/converters/network.hpp"
#include "swagger/converters/packet_generator.hpp"
#include "modules/dynamic/api.hpp"

namespace openperf::tvlp::api {

namespace dynamic = ::openperf::dynamic;

std::optional<time_point> from_rfc3339(const std::string& date)
{
    std::stringstream is(date);
    std::tm t = {};
    is >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    if (is.fail()) return std::nullopt;
    auto dur = std::chrono::system_clock::from_time_t(std::mktime(&t))
                   .time_since_epoch();

    // Calculate nanoseconds
    int d;
    double seconds = 0;
    sscanf(date.c_str(), "%d-%d-%dT%d:%d:%lfZ", &d, &d, &d, &d, &d, &seconds);

    auto chrono_sec = std::chrono::duration<double>(seconds);
    dur += std::chrono::duration_cast<std::chrono::nanoseconds>(
        chrono_sec
        - std::chrono::duration_cast<std::chrono::seconds>(chrono_sec));

    return time_point(dur);
}

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return (os.str());
}

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::TvlpStartConfiguration& config)
{
    auto errors = std::vector<std::string>();

    auto scale_check = [&errors](auto&& start_options) {
        if (start_options->getTimeScale() <= 0.0) {
            errors.emplace_back("Time scale value '"
                                + std::to_string(start_options->getTimeScale())
                                + "' should be greater than 0.0");
        }

        if (start_options->getLoadScale() <= 0.0) {
            errors.emplace_back("Load scale value '"
                                + std::to_string(start_options->getLoadScale())
                                + "' should be greater than 0.0");
        }
    };

    if (config.blockIsSet()) scale_check(config.getBlock());
    if (config.memoryIsSet()) scale_check(config.getMemory());
    if (config.cpuIsSet()) scale_check(config.getCpu());
    if (config.packetIsSet()) scale_check(config.getPacket());
    if (config.networkIsSet()) scale_check(config.getNetwork());
    if (config.startTimeIsSet()) {
        if (auto time = from_rfc3339(config.getStartTime()); !time)
            errors.emplace_back("Wrong start_time format");
    }

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

void apply_defaults(swagger::TvlpStartConfiguration& config)
{
    auto apply_scales = [](auto&& start_options) {
        if (!start_options->timeScaleIsSet()) start_options->setTimeScale(1.0);
        if (!start_options->loadScaleIsSet()) start_options->setLoadScale(1.0);
    };

    if (config.blockIsSet()) apply_scales(config.getBlock());
    if (config.memoryIsSet()) apply_scales(config.getMemory());
    if (config.cpuIsSet()) apply_scales(config.getCpu());
    if (config.packetIsSet()) apply_scales(config.getPacket());
    if (config.networkIsSet()) apply_scales(config.getNetwork());
}

tvlp_configuration_t from_swagger(const swagger::TvlpConfiguration& m)
{
    auto config = tvlp_configuration_t{};
    config.id(m.getId());

    tvlp_profile_t profile;
    if (m.getProfile()->blockIsSet()) {
        auto profile_model = m.getProfile()->getBlock();

        profile.block = tvlp_profile_t::series{};
        for (const auto& block_entry : profile_model->getSeries()) {
            profile.block.value().push_back(tvlp_profile_t::entry{
                .length = model::duration(block_entry->getLength()),
                .resource_id = block_entry->getResourceId(),
                .config = block_entry->getConfig()->toJson(),
            });
        }
    }

    if (m.getProfile()->memoryIsSet()) {
        auto profile_model = m.getProfile()->getMemory();

        profile.memory = tvlp_profile_t::series{};
        for (const auto& memory_entry : profile_model->getSeries()) {
            profile.memory.value().push_back(tvlp_profile_t::entry{
                .length = model::duration(memory_entry->getLength()),
                .config = memory_entry->getConfig()->toJson(),
            });
        }
    }

    if (m.getProfile()->cpuIsSet()) {
        auto profile_model = m.getProfile()->getCpu();

        profile.cpu = tvlp_profile_t::series{};
        for (const auto& cpu_entry : profile_model->getSeries()) {
            profile.cpu.value().push_back(tvlp_profile_t::entry{
                .length = model::duration(cpu_entry->getLength()),
                .config = cpu_entry->getConfig()->toJson(),
            });
        }
    }

    if (m.getProfile()->packetIsSet()) {
        auto profile_model = m.getProfile()->getPacket();

        profile.packet = tvlp_profile_t::series{};
        for (const auto& packet_entry : profile_model->getSeries()) {
            profile.packet.value().push_back(tvlp_profile_t::entry{
                .length = model::duration(packet_entry->getLength()),
                .resource_id = packet_entry->getTargetId(),
                .config = packet_entry->getConfig()->toJson(),
            });
        }
    }

    if (m.getProfile()->networkIsSet()) {
        auto profile_model = m.getProfile()->getNetwork();

        profile.network = tvlp_profile_t::series{};
        for (const auto& network_entry : profile_model->getSeries()) {
            profile.network.value().push_back(tvlp_profile_t::entry{
                .length = model::duration(network_entry->getLength()),
                .config = network_entry->getConfig()->toJson(),
            });
        }
    }

    config.profile(profile);
    return config;
}

tvlp_start_t from_swagger(const swagger::TvlpStartConfiguration& start)
{
    tvlp_start_t config;

    if (start.startTimeIsSet()) {
        config.start_time = from_rfc3339(start.getStartTime()).value();
    }

    if (start.memoryIsSet()) {
        auto memory = start.getMemory();
        auto start_opts = tvlp_start_t::start_t{
            .time_scale = memory->getTimeScale(),
            .load_scale = memory->getLoadScale(),
        };

        if (memory->dynamicResultsIsSet())
            start_opts.dynamic_results =
                dynamic::from_swagger(*memory->getDynamicResults());

        config.memory = start_opts;
    }

    if (start.cpuIsSet()) {
        auto cpu = start.getCpu();
        auto start_opts = tvlp_start_t::start_t{
            .time_scale = cpu->getTimeScale(),
            .load_scale = cpu->getLoadScale(),
        };

        if (cpu->dynamicResultsIsSet())
            start_opts.dynamic_results =
                dynamic::from_swagger(*cpu->getDynamicResults());

        config.cpu = start_opts;
    }

    if (start.blockIsSet()) {
        auto block = start.getBlock();
        auto start_opts = tvlp_start_t::start_t{
            .time_scale = block->getTimeScale(),
            .load_scale = block->getLoadScale(),
        };

        if (block->dynamicResultsIsSet())
            start_opts.dynamic_results =
                dynamic::from_swagger(*block->getDynamicResults());

        config.block = start_opts;
    }

    if (start.packetIsSet()) {
        auto packet = start.getPacket();
        auto start_opts = tvlp_start_t::start_t{
            .time_scale = packet->getTimeScale(),
            .load_scale = packet->getLoadScale(),
        };

        if (packet->dynamicResultsIsSet())
            start_opts.dynamic_results =
                dynamic::from_swagger(*packet->getDynamicResults());

        config.packet = start_opts;
    }

    if (start.networkIsSet()) {
        auto network = start.getNetwork();
        auto start_opts = tvlp_start_t::start_t{
            .time_scale = network->getTimeScale(),
            .load_scale = network->getLoadScale(),
        };

        if (network->dynamicResultsIsSet())
            start_opts.dynamic_results =
                dynamic::from_swagger(*network->getDynamicResults());

        config.network = start_opts;
    }

    return config;
}

swagger::TvlpConfiguration to_swagger(const tvlp_configuration_t& config)
{
    swagger::TvlpConfiguration model;
    model.setId(config.id());

    model.setTime(std::make_shared<swagger::TvlpConfiguration_time>());
    model.getTime()->setLength(config.total_length().count());

    if (config.state() == model::RUNNING || config.state() == model::COUNTDOWN)
        model.getTime()->setStart(
            to_rfc3339(config.start_time().time_since_epoch()));

    switch (config.state()) {
    case model::READY:
        model.setState("ready");
        break;
    case model::COUNTDOWN:
        model.setState("countdown");
        break;
    case model::RUNNING:
        model.getTime()->setOffset(config.current_offset().count());
        model.setState("running");
        break;
    case model::ERROR:
        model.setError(config.error());
        model.setState("error");
    }

    model.setProfile(std::make_shared<swagger::TvlpProfile>());
    if (config.profile().block) {
        auto block = std::make_shared<swagger::TvlpProfile_block>();
        auto block_vector = config.profile().block.value();
        std::transform(
            block_vector.begin(),
            block_vector.end(),
            std::back_inserter(block->getSeries()),
            [](auto& block_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_block_series>();
                entry->setLength(block_entry.length.count());
                entry->setResourceId(block_entry.resource_id.value());

                auto g_config =
                    std::make_shared<swagger::BlockGeneratorConfig>();
                g_config->fromJson(block_entry.config);
                entry->setConfig(g_config);
                return entry;
            });

        model.getProfile()->setBlock(block);
    }

    if (config.profile().memory) {
        auto memory = std::make_shared<swagger::TvlpProfile_memory>();
        auto memory_vector = config.profile().memory.value();
        std::transform(
            memory_vector.begin(),
            memory_vector.end(),
            std::back_inserter(memory->getSeries()),
            [](auto& memory_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_memory_series>();
                entry->setLength(memory_entry.length.count());

                auto g_config =
                    std::make_shared<swagger::MemoryGeneratorConfig>();
                g_config->fromJson(memory_entry.config);
                entry->setConfig(g_config);

                return entry;
            });
        model.getProfile()->setMemory(memory);
    }

    if (config.profile().cpu) {
        auto cpu = std::make_shared<swagger::TvlpProfile_cpu>();
        auto cpu_vector = config.profile().cpu.value();
        std::transform(
            cpu_vector.begin(),
            cpu_vector.end(),
            std::back_inserter(cpu->getSeries()),
            [](auto& cpu_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_cpu_series>();
                entry->setLength(cpu_entry.length.count());

                auto g_config = std::make_shared<swagger::CpuGeneratorConfig>();
                swagger::from_json(cpu_entry.config, *g_config);
                entry->setConfig(g_config);
                return entry;
            });
        model.getProfile()->setCpu(cpu);
    }

    if (config.profile().packet) {
        auto packet = std::make_shared<swagger::TvlpProfile_packet>();
        auto packet_vector = config.profile().packet.value();

        std::transform(
            packet_vector.begin(),
            packet_vector.end(),
            std::back_inserter(packet->getSeries()),
            [](auto& packet_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_packet_series>();
                entry->setLength(packet_entry.length.count());
                entry->setTargetId(packet_entry.resource_id.value());

                auto g_config =
                    std::make_shared<swagger::PacketGeneratorConfig>();
                swagger::from_json(packet_entry.config, *g_config);
                entry->setConfig(g_config);
                return entry;
            });
        model.getProfile()->setPacket(packet);
    }

    if (config.profile().network) {
        auto network = std::make_shared<swagger::TvlpProfile_network>();
        auto network_vector = config.profile().network.value();
        std::transform(
            network_vector.begin(),
            network_vector.end(),
            std::back_inserter(network->getSeries()),
            [](auto& network_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_network_series>();
                entry->setLength(network_entry.length.count());

                auto g_config =
                    std::make_shared<swagger::NetworkGeneratorConfig>();
                g_config->fromJson(network_entry.config);
                entry->setConfig(g_config);

                return entry;
            });
        model.getProfile()->setNetwork(network);
    }

    return model;
}

swagger::TvlpResult to_swagger(const tvlp_result_t& result)
{
    swagger::TvlpResult model;
    model.setId(result.id());
    model.setTvlpId(result.tvlp_id());

    auto modules_results = result.results();
    if (modules_results.block) {
        auto block_results = modules_results.block.value();
        std::transform(
            block_results.begin(),
            block_results.end(),
            std::back_inserter(model.getBlock()),
            [](auto& res) {
                auto g_result =
                    std::make_shared<swagger::BlockGeneratorResult>();
                swagger::from_json(res, *g_result);
                return g_result;
            });
    }

    if (modules_results.memory) {
        auto memory_results = modules_results.memory.value();
        std::transform(
            memory_results.begin(),
            memory_results.end(),
            std::back_inserter(model.getMemory()),
            [](auto& res) {
                auto g_result =
                    std::make_shared<swagger::MemoryGeneratorResult>();
                swagger::from_json(res, *g_result);
                return g_result;
            });
    }

    if (modules_results.cpu) {
        auto cpu_results = modules_results.cpu.value();
        std::transform(cpu_results.begin(),
                       cpu_results.end(),
                       std::back_inserter(model.getCpu()),
                       [](auto& res) {
                           auto g_result =
                               std::make_shared<swagger::CpuGeneratorResult>();
                           swagger::from_json(res, *g_result);
                           return g_result;
                       });
    }

    if (modules_results.packet) {
        auto packet_results = modules_results.packet.value();
        std::transform(
            packet_results.begin(),
            packet_results.end(),
            std::back_inserter(model.getPacket()),
            [](auto& res) {
                auto g_result =
                    std::make_shared<swagger::PacketGeneratorResult>();
                swagger::from_json(res, *g_result);
                return g_result;
            });
    }

    if (modules_results.network) {
        auto network_results = modules_results.network.value();
        std::transform(
            network_results.begin(),
            network_results.end(),
            std::back_inserter(model.getNetwork()),
            [](auto& res) {
                auto g_result =
                    std::make_shared<swagger::NetworkGeneratorResult>();
                swagger::from_json(res, *g_result);
                return g_result;
            });
    }

    return model;
}

} // namespace openperf::tvlp::api
