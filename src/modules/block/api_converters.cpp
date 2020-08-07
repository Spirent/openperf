#include "api_converters.hpp"

#include <iomanip>
#include <sstream>

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, BlockFile& file)
{
    file.setFileSize(j.at("file_size"));
    file.setPath(j.at("path"));

    if (j.find("id") != j.end()) file.setId(j.at("id"));

    if (j.find("init_percent_complete") != j.end())
        file.setInitPercentComplete(j.at("init_percent_complete"));

    if (j.find("state") != j.end()) file.setState(j.at("state"));
}

void from_json(const nlohmann::json& j, BulkCreateBlockFilesRequest& request)
{
    request.getItems().clear();
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<BlockFile> newItem(new BlockFile());
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j, BulkDeleteBlockFilesRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BlockGenerator& generator)
{
    generator.setResourceId(j.at("resource_id"));

    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }
    if (j.find("running") != j.end()) { generator.setRunning(j.at("running")); }

    auto gc = BlockGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<BlockGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j,
               BulkCreateBlockGeneratorsRequest& request)
{
    request.getItems().clear();
    for (auto& item : const_cast<nlohmann::json&>(j).at("items")) {
        if (item.is_null()) {
            continue;
        } else {
            std::shared_ptr<BlockGenerator> newItem(new BlockGenerator());
            from_json(item, *newItem);
            request.getItems().push_back(newItem);
        }
    }
}

void from_json(const nlohmann::json& j,
               BulkDeleteBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j,
               BulkStartBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BulkStopBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model

namespace openperf::block::api {

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return (os.str());
}

constexpr model::block_generation_pattern
to_block_generation_pattern(std::string_view value)
{
    if (value == "random") return model::block_generation_pattern::RANDOM;
    if (value == "sequential")
        return model::block_generation_pattern::SEQUENTIAL;
    if (value == "reverse") return model::block_generation_pattern::REVERSE;

    throw std::runtime_error("Pattern \"" + std::string(value)
                             + "\" is unknown");
}

constexpr std::string_view to_string(model::block_generation_pattern pattern)
{
    switch (pattern) {
    case model::block_generation_pattern::RANDOM:
        return "random";
    case model::block_generation_pattern::SEQUENTIAL:
        return "sequential";
    case model::block_generation_pattern::REVERSE:
        return "reverse";
    default:
        return "unknown";
    };
}

constexpr std::string_view to_string(model::device::state_t state)
{
    switch (state) {
    case model::device::state_t::UNINIT:
        return "uninitialized";
    case model::device::state_t::INIT:
        return "initializing";
    case model::device::state_t::READY:
        return "ready";
    default:
        return "unknown";
    }
}

constexpr model::file::state_t to_block_file_state(std::string_view value)
{
    if (value == "none") return model::file::state_t::NONE;
    if (value == "init") return model::file::state_t::INIT;
    if (value == "ready") return model::file::state_t::READY;

    throw std::runtime_error("Pattern \"" + std::string(value)
                             + "\" is unknown");
}

constexpr std::string_view to_string(model::file::state_t state)
{
    switch (state) {
    case model::file::state_t::NONE:
        return "none";
    case model::file::state_t::INIT:
        return "init";
    case model::file::state_t::READY:
        return "ready";
    default:
        return "unknown";
    };
}

bool is_valid(const BlockFile& file, std::vector<std::string>& errors)
{
    auto init_errors = errors.size();

    if (file.getFileSize() <= 0) {
        errors.emplace_back("File size is not valid.");
    }

    return (errors.size() == init_errors);
}

bool is_valid(const BlockGenerator& generator, std::vector<std::string>& errors)
{
    auto init_errors = errors.size();

    if (generator.getResourceId().empty()) {
        errors.emplace_back("Generator resource id is required.");
    }

    auto config = generator.getConfig();
    if (!config) {
        errors.emplace_back("Generator configuration is required.");
        return (false);
    }

    if (config->getQueueDepth() < 1)
        errors.emplace_back("Queue Depth value is not valid.");
    if (config->getReadSize() < 1)
        errors.emplace_back("Read Size value is not valid.");
    if (config->getReadsPerSec() < 0)
        errors.emplace_back("Reads Per Sec value is not valid.");
    if (config->getWriteSize() < 1)
        errors.emplace_back("Write Size value is not valid.");
    if (config->getWritesPerSec() < 0)
        errors.emplace_back("Writes Per Sec value is not valid.");
    if (config->ratioIsSet()) {
        if (config->getRatio()->getReads() < 1)
            errors.emplace_back("Ratio Reads value is not valid.");
        if (config->getRatio()->getWrites() < 1)
            errors.emplace_back("Ratio Writes value is not valid.");
    }
    if (config->getReadsPerSec() < 1 && config->getWritesPerSec() < 1)
        errors.emplace_back("No operations were specified.");
    if (config->ratioIsSet()
        && (config->getReadsPerSec() < 1 || config->getWritesPerSec() < 1))
        errors.emplace_back("Ratio is specified for empty load generation.");

    assert(config);

    return (errors.size() == init_errors);
}

std::shared_ptr<BlockDevice> to_swagger(const model::device& p_device)
{
    auto device = std::make_shared<BlockDevice>();
    device->setId(p_device.id());
    device->setInfo(p_device.info());
    device->setPath(p_device.path());
    device->setSize(p_device.size());
    device->setUsable(p_device.is_usable());
    device->setInitPercentComplete(p_device.init_percent_complete());
    device->setState(std::string(to_string(p_device.state())));
    return device;
}

std::shared_ptr<BlockFile> to_swagger(const model::file& p_file)
{
    auto blkfile = std::make_shared<BlockFile>();
    blkfile->setId(p_file.id());
    blkfile->setPath(p_file.path());
    blkfile->setFileSize(p_file.size());
    blkfile->setInitPercentComplete(p_file.init_percent_complete());
    blkfile->setState(std::string(to_string(p_file.state())));
    return blkfile;
}

std::shared_ptr<BlockGenerator> to_swagger(const model::block_generator& p_gen)
{
    auto gen_config = std::make_shared<BlockGeneratorConfig>();
    gen_config->setPattern(std::string(to_string(p_gen.config().pattern)));
    gen_config->setQueueDepth(p_gen.config().queue_depth);
    gen_config->setReadSize(p_gen.config().read_size);
    gen_config->setReadsPerSec(p_gen.config().reads_per_sec);
    gen_config->setWriteSize(p_gen.config().write_size);
    gen_config->setWritesPerSec(p_gen.config().writes_per_sec);
    if (p_gen.config().ratio) {
        auto ratio = std::make_shared<BlockGeneratorReadWriteRatio>();
        ratio->setReads(p_gen.config().ratio.value().reads);
        ratio->setWrites(p_gen.config().ratio.value().writes);
        gen_config->setRatio(ratio);
    }

    auto gen = std::make_shared<BlockGenerator>();
    gen->setId(p_gen.id());
    gen->setResourceId(p_gen.resource_id());
    gen->setRunning(p_gen.is_running());
    gen->setConfig(gen_config);

    return gen;
}

std::shared_ptr<BlockGeneratorResult>
to_swagger(const model::block_generator_result& p_gen_result)
{
    auto gen_res = std::make_shared<BlockGeneratorResult>();
    gen_res->setId(p_gen_result.id());
    gen_res->setGeneratorId(p_gen_result.generator_id());
    gen_res->setActive(p_gen_result.is_active());
    gen_res->setTimestamp(
        to_rfc3339(p_gen_result.timestamp().time_since_epoch()));

    auto to_statistics_t =
        [](const model::block_generator_result::statistics_t& gen_stat) {
            auto stat = std::make_shared<BlockGeneratorStats>();
            stat->setBytesActual(gen_stat.bytes_actual);
            stat->setBytesTarget(gen_stat.bytes_target);
            stat->setIoErrors(gen_stat.io_errors);
            stat->setOpsActual(gen_stat.ops_actual);
            stat->setOpsTarget(gen_stat.ops_target);
            stat->setLatencyTotal(gen_stat.latency.count());

            if (gen_stat.latency_max.has_value())
                stat->setLatencyMin(gen_stat.latency_min.value().count());

            if (gen_stat.latency_min.has_value())
                stat->setLatencyMax(gen_stat.latency_max.value().count());

            return stat;
        };

    gen_res->setRead(to_statistics_t(p_gen_result.read_stats()));
    gen_res->setWrite(to_statistics_t(p_gen_result.write_stats()));

    return gen_res;
}

model::file from_swagger(const BlockFile& p_file)
{
    model::file f;
    f.id(p_file.getId());
    f.path(p_file.getPath());
    f.size(p_file.getFileSize());
    f.init_percent_complete(p_file.getInitPercentComplete());
    return f;
}

model::block_generator from_swagger(const BlockGenerator& p_gen)
{
    model::block_generator gen;
    gen.id(p_gen.getId());
    gen.resource_id(p_gen.getResourceId());
    gen.running(p_gen.isRunning());

    auto conf = model::block_generator_config{
        .queue_depth =
            static_cast<uint32_t>(p_gen.getConfig()->getQueueDepth()),
        .reads_per_sec =
            static_cast<uint32_t>(p_gen.getConfig()->getReadsPerSec()),
        .read_size = static_cast<uint32_t>(p_gen.getConfig()->getReadSize()),
        .writes_per_sec =
            static_cast<uint32_t>(p_gen.getConfig()->getWritesPerSec()),
        .write_size = static_cast<uint32_t>(p_gen.getConfig()->getWriteSize()),
        .pattern = to_block_generation_pattern(p_gen.getConfig()->getPattern()),
    };

    if (p_gen.getConfig()->ratioIsSet()) {
        conf.ratio = model::block_generator_ratio{
            .reads = static_cast<uint32_t>(
                p_gen.getConfig()->getRatio()->getReads()),
            .writes = static_cast<uint32_t>(
                p_gen.getConfig()->getRatio()->getWrites()),
        };
    }

    gen.config(conf);
    return gen;
}

request_block_file_bulk_add from_swagger(BulkCreateBlockFilesRequest& p_request)
{
    request_block_file_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.files.emplace_back(
            std::make_unique<model::file>(from_swagger(*item)));
    return request;
}

request_block_file_bulk_del from_swagger(BulkDeleteBlockFilesRequest& p_request)
{
    request_block_file_bulk_del request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<std::string>(id));
    return request;
}

request_block_generator_bulk_add
from_swagger(BulkCreateBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.generators.emplace_back(
            std::make_unique<model::block_generator>(from_swagger(*item)));
    return request;
}

request_block_generator_bulk_del
from_swagger(BulkDeleteBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_del request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<std::string>(id));
    return request;
}

request_block_generator_bulk_start
from_swagger(BulkStartBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_start request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<std::string>(id));
    return request;
}

request_block_generator_bulk_stop
from_swagger(BulkStopBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_stop request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<std::string>(id));
    return request;
}

} // namespace openperf::block::api
