#include "api_converters.hpp"

#include <iomanip>

#include "swagger/converters/block.hpp"
#include "swagger/converters/memory.hpp"
#include "swagger/converters/cpu.hpp"
#include "swagger/converters/packet_generator.hpp"

namespace openperf::tvlp::api {

std::optional<time_point> from_rfc3339(const std::string& from)
{
    std::string date = from;
    while (true) {
        auto idx = date.find("%3A");
        if (idx == std::string::npos) break;
        date.replace(idx, 3, ":");
    }

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
is_valid(const swagger::TvlpConfiguration& config)
{
    auto errors = std::vector<std::string>();

    auto scale_check = [&errors](auto&& profile) {
        if (profile->getTimeScale() <= 0.0) {
            errors.emplace_back("Time scale value '"
                                + std::to_string(profile->getTimeScale())
                                + "' should be greater than 0.0");
        }

        if (profile->getLoadScale() <= 0.0) {
            errors.emplace_back("Load scale value '"
                                + std::to_string(profile->getLoadScale())
                                + "' should be greater than 0.0");
        }
    };

    if (config.getProfile()->blockIsSet())
        scale_check(config.getProfile()->getBlock());

    if (config.getProfile()->memoryIsSet())
        scale_check(config.getProfile()->getMemory());

    if (config.getProfile()->cpuIsSet())
        scale_check(config.getProfile()->getCpu());

    if (config.getProfile()->packetIsSet())
        scale_check(config.getProfile()->getPacket());

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

tvlp_configuration_t from_swagger(const swagger::TvlpConfiguration& m)
{
    auto config = tvlp_configuration_t{};
    config.id(m.getId());

    tvlp_profile_t profile;
    if (m.getProfile()->blockIsSet()) {
        auto profile_model = m.getProfile()->getBlock();
        auto time_scale = profile_model->getTimeScale();
        auto load_scale = profile_model->getLoadScale();

        profile.block = std::vector<tvlp_profile_entry_t>();
        for (const auto& block_entry : profile_model->getSeries()) {
            profile.block.value().push_back(tvlp_profile_entry_t{
                .length = model::duration(block_entry->getLength()),
                .resource_id = block_entry->getResourceId(),
                .config = block_entry->getConfig()->toJson(),
                .time_scale = time_scale,
                .load_scale = load_scale,
            });
        }
    }

    if (m.getProfile()->memoryIsSet()) {
        auto profile_model = m.getProfile()->getMemory();
        auto time_scale = profile_model->getTimeScale();
        auto load_scale = profile_model->getLoadScale();

        profile.memory = std::vector<tvlp_profile_entry_t>();
        for (const auto& memory_entry : profile_model->getSeries()) {
            profile.memory.value().push_back(tvlp_profile_entry_t{
                .length = model::duration(memory_entry->getLength()),
                .config = memory_entry->getConfig()->toJson(),
                .time_scale = time_scale,
                .load_scale = load_scale,
            });
        }
    }

    if (m.getProfile()->cpuIsSet()) {
        auto profile_model = m.getProfile()->getCpu();
        auto time_scale = profile_model->getTimeScale();
        auto load_scale = profile_model->getLoadScale();

        profile.cpu = std::vector<tvlp_profile_entry_t>();
        for (const auto& cpu_entry : profile_model->getSeries()) {
            profile.cpu.value().push_back(tvlp_profile_entry_t{
                .length = model::duration(cpu_entry->getLength()),
                .config = cpu_entry->getConfig()->toJson(),
                .time_scale = time_scale,
                .load_scale = load_scale,
            });
        }
    }

    if (m.getProfile()->packetIsSet()) {
        auto profile_model = m.getProfile()->getPacket();
        auto time_scale = profile_model->getTimeScale();
        auto load_scale = profile_model->getLoadScale();

        profile.packet = std::vector<tvlp_profile_entry_t>();
        for (const auto& packet_entry : profile_model->getSeries()) {
            profile.packet.value().push_back(tvlp_profile_entry_t{
                .length = model::duration(packet_entry->getLength()),
                .resource_id = packet_entry->getTargetId(),
                .config = packet_entry->getConfig()->toJson(),
                .time_scale = time_scale,
                .load_scale = load_scale,
            });
        }
    }

    config.profile(profile);
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
            [&block](auto& block_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_block_series>();
                entry->setLength(block_entry.length.count());
                entry->setResourceId(block_entry.resource_id.value());

                block->setTimeScale(block_entry.time_scale);
                block->setLoadScale(block_entry.load_scale);

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
            [&memory](auto& memory_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_memory_series>();
                entry->setLength(memory_entry.length.count());

                memory->setTimeScale(memory_entry.time_scale);
                memory->setLoadScale(memory_entry.load_scale);

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
            [&cpu](auto& cpu_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_cpu_series>();
                entry->setLength(cpu_entry.length.count());

                cpu->setTimeScale(cpu_entry.time_scale);
                cpu->setLoadScale(cpu_entry.load_scale);

                auto g_config = std::make_shared<swagger::CpuGeneratorConfig>();
                g_config->fromJson(cpu_entry.config);
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
            [&packet](auto& packet_entry) {
                auto entry =
                    std::make_shared<swagger::TvlpProfile_packet_series>();
                entry->setLength(packet_entry.length.count());
                entry->setTargetId(packet_entry.resource_id.value());

                packet->setTimeScale(packet_entry.time_scale);
                packet->setLoadScale(packet_entry.load_scale);

                auto g_config =
                    std::make_shared<swagger::PacketGeneratorConfig>();
                swagger::from_json(packet_entry.config, *g_config);
                entry->setConfig(g_config);
                return entry;
            });
        model.getProfile()->setPacket(packet);
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

    return model;
}

} // namespace openperf::tvlp::api
