#ifndef _OP_BLOCK_UTILS_HPP_
#define _OP_BLOCK_UTILS_HPP_

#include <sstream>
#include <iomanip>
#include "timesync/bintime.hpp"
#include "timesync/chrono.hpp"

#include "swagger/v1/model/BlockGenerator.h"
#include "swagger/v1/model/BlockGeneratorResult.h"
#include "swagger/v1/model/BlockFile.h"
#include "swagger/v1/model/BlockDevice.h"

#include "models/device.hpp"
#include "models/file.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

using namespace swagger::v1::model;
using namespace openperf::block::model;

const static std::unordered_map<block_generation_pattern, std::string> block_generation_pattern_strings = {
    {block_generation_pattern::RANDOM, "random"},
    {block_generation_pattern::SEQUENTIAL, "sequential"},
    {block_generation_pattern::REVERSE, "reverse"},
};

const static std::unordered_map<std::string, block_generation_pattern> block_generation_patterns = {
    {"random", block_generation_pattern::RANDOM},
    {"sequential", block_generation_pattern::SEQUENTIAL},
    {"reverse", block_generation_pattern::REVERSE},
};

std::string to_string(const block_generation_pattern& pattern) {
    if (block_generation_pattern_strings.count(pattern))
        return block_generation_pattern_strings.at(pattern);
    return "unknown";
}

block_generation_pattern block_generation_pattern_from_string(const std::string& value) {
    if (block_generation_patterns.count(value))
        return block_generation_patterns.at(value);
    throw std::runtime_error("Pattern \"" + value + "\" is unknown");
}



static std::shared_ptr<BlockGenerator>
make_swagger(const block_generator& p_generator)
{
    auto generator = std::make_shared<BlockGenerator>();
    generator->setId(p_generator.get_id());
    generator->setResourceId(p_generator.get_resource_id());
    generator->setRunning(p_generator.is_running());

    auto config = std::make_shared<BlockGeneratorConfig>();
    config->setPattern(to_string(p_generator.get_config().pattern));
    config->setQueueDepth(p_generator.get_config().queue_depth);
    config->setReadSize(p_generator.get_config().read_size);
    config->setReadsPerSec(p_generator.get_config().reads_per_sec);
    config->setWriteSize(p_generator.get_config().write_size);
    config->setWritesPerSec(p_generator.get_config().writes_per_sec);

    generator->setConfig(config);

    return generator;
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

static std::shared_ptr<BlockGeneratorResult>
make_swagger(const block_generator_result& p_gen_result)
{
    auto gen_res = std::make_shared<BlockGeneratorResult>();
    gen_res->setId(p_gen_result.get_id());
    gen_res->setActive(p_gen_result.is_active());
    gen_res->setTimestamp(to_rfc3339(p_gen_result.get_timestamp().time_since_epoch()));

    auto generate_gen_stat = [](const block_generator_statistics& gen_stat){
        auto stat = std::make_shared<BlockGeneratorStats>();
        stat->setBytesActual(gen_stat.bytes_actual);
        stat->setBytesTarget(gen_stat.bytes_target);
        stat->setIoErrors(gen_stat.io_errors);
        stat->setOpsActual(gen_stat.ops_actual);
        stat->setOpsTarget(gen_stat.ops_target);
        stat->setLatency(gen_stat.latency);
        stat->setLatencyMin(gen_stat.latency_min);
        stat->setLatencyMax(gen_stat.latency_max);
        return stat;
    };

    gen_res->setRead(generate_gen_stat(p_gen_result.get_read_stats()));
    gen_res->setWrite(generate_gen_stat(p_gen_result.get_write_stats()));

    return gen_res;
}

#endif // _OP_BLOCK_UTILS_HPP_