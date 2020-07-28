#include "api_converters.hpp"

#include <iomanip>

namespace openperf::memory::api {

namespace swagger = ::swagger::v1::model;
using config_t = memory::internal::generator::config_t;
using memory_stat = memory::internal::memory_stat;
using task_memory_stat = memory::internal::task_memory_stat;
using info_t = memory::memory_info::info_t;
using io_pattern = memory::internal::io_pattern;

constexpr io_pattern from_string(std::string_view str)
{
    if (str == "random") return io_pattern::RANDOM;
    if (str == "sequential") return io_pattern::SEQUENTIAL;
    if (str == "reverse") return io_pattern::REVERSE;
    return io_pattern::NONE;
}

constexpr std::string_view to_string(io_pattern pattern)
{
    switch (pattern) {
    case io_pattern::RANDOM:
        return "random";
    case io_pattern::REVERSE:
        return "reverse";
    case io_pattern::SEQUENTIAL:
        return "sequential";
    default:
        return "";
    }
}

std::string to_iso8601(const task_memory_stat::timestamp_t& ts)
{
    auto t = task_memory_stat::clock::to_time_t(ts);
    auto nanos = std::chrono::time_point_cast<std::chrono::nanoseconds>(ts)
                 - std::chrono::time_point_cast<std::chrono::seconds>(ts);
    std::stringstream os;
    os << std::put_time(gmtime(&t), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << nanos.count() << "Z";

    return os.str();
}

config_t from_swagger(const swagger::MemoryGeneratorConfig& m)
{
    return config_t{
        .buffer_size = static_cast<size_t>(m.getBufferSize()),
        .read =
            {
                .block_size = static_cast<size_t>(m.getReadSize()),
                .op_per_sec = static_cast<size_t>(m.getReadsPerSec()),
                .threads = static_cast<size_t>(m.getReadThreads()),
                .pattern = from_string(m.getPattern()),
            },
        .write = {
            .block_size = static_cast<size_t>(m.getWriteSize()),
            .op_per_sec = static_cast<size_t>(m.getWritesPerSec()),
            .threads = static_cast<size_t>(m.getWriteThreads()),
            .pattern = from_string(m.getPattern()),
        }};
}

swagger::MemoryGeneratorConfig to_swagger(const config_t& config)
{
    swagger::MemoryGeneratorConfig model;
    model.setBufferSize(config.buffer_size);

    model.setReadSize(config.read.block_size);
    model.setReadsPerSec(config.read.op_per_sec);
    model.setReadThreads(config.read.threads);

    model.setWriteSize(config.write.block_size);
    model.setWritesPerSec(config.write.op_per_sec);
    model.setWriteThreads(config.write.threads);

    model.setPattern(std::string(to_string(config.read.pattern)));

    return model;
}

swagger::MemoryGenerator to_swagger(const reply::generator::item::item_data& g)
{
    swagger::MemoryGenerator model;
    model.setId(g.id);
    model.setRunning(g.is_running);
    model.setInitPercentComplete(g.init_percent_complete);
    model.setConfig(
        std::make_shared<swagger::MemoryGeneratorConfig>(to_swagger(g.config)));

    return model;
}

swagger::MemoryGeneratorStats to_swagger(const task_memory_stat& stat)
{
    swagger::MemoryGeneratorStats model;
    model.setBytesActual(stat.bytes);
    model.setBytesTarget(stat.bytes_target);
    model.setLatency(
        std::chrono::duration_cast<std::chrono::nanoseconds>(stat.run_time)
            .count());
    model.setOpsActual(stat.operations);
    model.setOpsTarget(stat.operations_target);
    model.setIoErrors(stat.errors);

    if (stat.latency_max.has_value()) {
        model.setLatencyMax(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                stat.latency_max.value())
                .count());
    }

    if (stat.latency_min.has_value()) {
        model.setLatencyMin(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                stat.latency_min.value())
                .count());
    }

    return model;
}

swagger::MemoryGeneratorResult
to_swagger(const reply::statistic::item::item_data& i)
{
    swagger::MemoryGeneratorResult model;
    model.setId(i.id);
    model.setActive(i.stat.active);
    model.setGeneratorId(i.generator_id);
    model.setTimestamp(to_iso8601(i.stat.timestamp()));
    model.setRead(std::make_shared<swagger::MemoryGeneratorStats>(
        to_swagger(i.stat.read)));
    model.setWrite(std::make_shared<swagger::MemoryGeneratorStats>(
        to_swagger(i.stat.write)));

    return model;
}

swagger::MemoryInfoResult to_swagger(const info_t& info)
{
    swagger::MemoryInfoResult model;
    model.setFreeMemory(info.free);
    model.setTotalMemory(info.total);

    return model;
}

} // namespace openperf::memory::api