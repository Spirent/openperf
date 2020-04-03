#ifndef _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
#define _OP_MEMORY_SWAGGER_CONVERTERS_HPP_

#include <iomanip>

#include "memory/generator.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"
#include "swagger/v1/model/MemoryGeneratorStats.h"

namespace openperf::memory {

namespace swagger = ::swagger::v1::model;

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

static generator::config_t from_swagger(const model::MemoryGeneratorConfig& m)
{
    return generator::config_t {
        .buffer_size = static_cast<size_t>(m.getBufferSize()),
        .read_threads = static_cast<size_t>(m.getReadThreads()),
        .write_threads = static_cast<size_t>(m.getWriteThreads()),
        .read = {
            .block_size = static_cast<size_t>(m.getReadSize()),
            .op_per_sec = static_cast<size_t>(m.getReadsPerSec()),
            .pattern = from_string(m.getPattern()),
        },
        .write = {
            .block_size = static_cast<size_t>(m.getWriteSize()),
            .op_per_sec = static_cast<size_t>(m.getWritesPerSec()),
            .pattern = from_string(m.getPattern()),
        }
    };
}

static swagger::MemoryGeneratorConfig to_swagger(const generator::config_t& config)
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

static std::string to_iso8601(uint64_t nanos)
{
    std::cout << "Nanos: " << nanos << std::endl;
    std::chrono::nanoseconds ns(nanos);
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::time_point(ns));
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(ns);
    std::stringstream os;
    os << std::put_time(gmtime(&t), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << (ns - sec).count() << "Z";

    std::cout << "Time: " << os.str() << std::endl;
    return os.str();
}

} // namespace openperf::memory

#endif // _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
