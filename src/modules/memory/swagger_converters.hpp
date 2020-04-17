#ifndef _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
#define _OP_MEMORY_SWAGGER_CONVERTERS_HPP_

#include <iomanip>

#include "memory/info.hpp"
#include "memory/generator.hpp"
#include "swagger/v1/model/MemoryInfoResult.h"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"
#include "swagger/v1/model/MemoryGeneratorStats.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"

namespace openperf::memory::api {

namespace swagger = ::swagger::v1::model;
using config_t = memory::internal::generator::config_t;
using stat_t = memory::internal::generator::stat_t;
using memory_stat = memory::internal::memory_stat;
using info_t = memory::memory_info::info_t;
using io_pattern = memory::internal::io_pattern;

namespace {
static io_pattern from_string(std::string_view str)
{
    if (str == "random") return io_pattern::RANDOM;
    if (str == "sequential") return io_pattern::SEQUENTIAL;
    if (str == "reverse") return io_pattern::REVERSE;
    return io_pattern::NONE;
}

static std::string to_string(io_pattern pattern)
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
} // namespace

static config_t from_swagger(const swagger::MemoryGeneratorConfig& m)
{
    return config_t{
        .buffer_size = static_cast<size_t>(m.getBufferSize()),
        .read_threads = static_cast<size_t>(m.getReadThreads()),
        .write_threads = static_cast<size_t>(m.getWriteThreads()),
        .read =
            {
                .block_size = static_cast<size_t>(m.getReadSize()),
                .op_per_sec = static_cast<size_t>(m.getReadsPerSec()),
                .pattern = from_string(m.getPattern()),
            },
        .write = {
            .block_size = static_cast<size_t>(m.getWriteSize()),
            .op_per_sec = static_cast<size_t>(m.getWritesPerSec()),
            .pattern = from_string(m.getPattern()),
        }};
}

static swagger::MemoryGeneratorConfig to_swagger(const config_t& config)
{
    swagger::MemoryGeneratorConfig model;
    model.setBufferSize(config.buffer_size);
    model.setReadThreads(config.read_threads);
    model.setWriteThreads(config.write_threads);

    model.setReadSize(config.read.block_size);
    model.setReadsPerSec(config.read.op_per_sec);

    model.setWriteSize(config.write.block_size);
    model.setWritesPerSec(config.write.op_per_sec);

    model.setPattern(to_string(config.read.pattern));

    return model;
}

static swagger::MemoryGenerator
to_swagger(const reply::generator::item::item_data& g)
{
    swagger::MemoryGenerator model;
    model.setId(g.id);
    model.setRunning(g.is_running);
    model.setConfig(
        std::make_shared<swagger::MemoryGeneratorConfig>(to_swagger(g.config)));

    return model;
}

static swagger::MemoryGeneratorStats to_swagger(const memory_stat& stat)
{
    swagger::MemoryGeneratorStats model;
    model.setBytesActual(stat.bytes);
    model.setBytesTarget(stat.bytes_target);
    model.setIoErrors(stat.errors);
    model.setLatency(stat.time_ns);
    model.setLatencyMax(stat.latency_max);
    model.setLatencyMin(stat.latency_min);
    model.setOpsActual(stat.operations);
    model.setOpsTarget(stat.operations_target);

    return model;
}

static std::string to_iso8601(const memory_stat::timestamp_t& ts)
{
    auto t = std::chrono::system_clock::to_time_t(ts);
    auto nanos = std::chrono::time_point_cast<std::chrono::nanoseconds>(ts)
                 - std::chrono::time_point_cast<std::chrono::seconds>(ts);
    std::stringstream os;
    os << std::put_time(gmtime(&t), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << nanos.count() << "Z";

    return os.str();
}

static swagger::MemoryGeneratorResult
to_swagger(const reply::statistic::item::item_data& i)
{
    swagger::MemoryGeneratorResult model;
    model.setId(i.id);
    model.setActive(i.stat.active);
    model.setGeneratorId(i.generator_id);
    model.setTimestamp(to_iso8601(i.stat.timestamp));
    model.setRead(std::make_shared<swagger::MemoryGeneratorStats>(
        to_swagger(i.stat.read)));
    model.setWrite(std::make_shared<swagger::MemoryGeneratorStats>(
        to_swagger(i.stat.write)));

    return model;
}

static swagger::MemoryInfoResult to_swagger(const info_t& info)
{
    swagger::MemoryInfoResult model;
    model.setFreeMemory(info.free);
    model.setTotalMemory(info.total);

    return model;
}

} // namespace openperf::memory::api

#endif // _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
