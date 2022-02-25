#include <iomanip>

#include "memory/api.hpp"
#include "memory/generator/config.hpp"
#include "memory/generator/coordinator.hpp"

#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"

namespace openperf::memory::api {

static std::shared_ptr<swagger::v1::model::MemoryGeneratorConfig>
to_swagger(const generator::config& config)
{
    auto dst = std::make_shared<swagger::v1::model::MemoryGeneratorConfig>();
    dst->setBufferSize(config.buffer_size);
    dst->setReadsPerSec(
        static_cast<int64_t>(std::trunc(config.read.io_rate.count())));
    dst->setReadSize(config.read.io_size);
    dst->setReadThreads(config.read.io_threads);
    dst->setWritesPerSec(
        static_cast<int64_t>(std::trunc(config.write.io_rate.count())));
    dst->setWriteSize(config.write.io_size);
    dst->setWriteThreads(config.write.io_threads);
    dst->setPattern(to_string(config.read.io_pattern));

    return (dst);
}

mem_generator_ptr to_swagger(std::string_view id,
                             const generator::coordinator& gen)
{
    auto dst = std::make_unique<swagger::v1::model::MemoryGenerator>();
    dst->setId(std::string(id));
    dst->setConfig(to_swagger(gen.config()));
    dst->setRunning(gen.active());
    dst->setInitPercentComplete(100);

    return (dst);
}

template <typename Clock>
std::string to_rfc3339(std::chrono::time_point<Clock> from)
{
    using namespace std::chrono_literals;
    static constexpr auto ns_per_sec = std::chrono::nanoseconds(1s).count();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        from.time_since_epoch())
                        .count();

    time_t sec = total_ns / ns_per_sec;
    auto ns = total_ns % ns_per_sec;

    std::stringstream os;
    os << std::put_time(gmtime(&sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(9) << ns << "Z";
    return (os.str());
}

template <typename Duration>
std::chrono::nanoseconds to_nanoseconds(const Duration& duration)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
}

template <typename Clock>
static std::shared_ptr<swagger::v1::model::MemoryGeneratorStats>
to_swagger(const generator::io_stats<Clock>& stats)
{
    auto dst = std::make_shared<swagger::v1::model::MemoryGeneratorStats>();

    dst->setOpsTarget(stats.ops_target);
    dst->setOpsActual(stats.ops_actual);
    dst->setBytesTarget(stats.bytes_target);
    dst->setBytesActual(stats.bytes_actual);
    dst->setLatencyTotal(to_nanoseconds(stats.latency).count());

    return (dst);
}

mem_generator_result_ptr
to_swagger(std::string_view generator_id,
           std::string_view result_id,
           const std::shared_ptr<generator::result>& result)
{
    auto dst = std::make_unique<swagger::v1::model::MemoryGeneratorResult>();
    dst->setId(std::string(result_id));
    dst->setGeneratorId(std::string(generator_id));

    /* Result is active iff coordinator and server both have pointers to it */
    dst->setActive(result.use_count() > 1);

    auto stats = generator::sum_stats(result->shards());

    dst->setTimestampFirst(to_rfc3339(stats.time_.first));
    dst->setTimestampLast(to_rfc3339(stats.time_.last));
    dst->setRead(to_swagger(stats.read));
    dst->setWrite(to_swagger(stats.write));
    dst->setDynamicResults(std::make_shared<swagger::v1::model::DynamicResults>(
        to_swagger(result->dynamic())));

    return (dst);
}

generator::config
from_swagger(const swagger::v1::model::MemoryGeneratorConfig& config)
{
    auto dst = generator::config{
        .buffer_size = static_cast<uint64_t>(config.getBufferSize()),
        .read =
            {
                .io_rate = generator::ops_per_sec{config.getReadsPerSec()},
                .io_size = static_cast<unsigned>(config.getReadSize()),
                .io_threads = static_cast<unsigned>(config.getReadThreads()),
                .io_pattern = to_pattern_type(config.getPattern()),
            },
        .write = {
            .io_rate = generator::ops_per_sec{config.getWritesPerSec()},
            .io_size = static_cast<unsigned>(config.getWriteSize()),
            .io_threads = static_cast<unsigned>(config.getWriteThreads()),
            .io_pattern = to_pattern_type(config.getPattern()),
        }};

    return (dst);
}

} // namespace openperf::memory::api
