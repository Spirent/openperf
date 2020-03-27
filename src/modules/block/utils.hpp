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

static std::shared_ptr<BlockDevice>
make_swagger(const device& p_device)
{
    auto device = std::make_shared<BlockDevice>();
    device->setId(p_device.get_id());
    device->setInfo(p_device.get_info());
    device->setPath(p_device.get_path());
    device->setSize(p_device.get_size());
    device->setUsable(p_device.is_usable());
    return device;
}

static std::shared_ptr<BlockFile>
make_swagger(const file& p_file)
{
    auto f = std::make_shared<BlockFile>();
    f->setId(p_file.get_id());
    f->setPath(p_file.get_path());
    f->setState(p_file.get_state());
    f->setFileSize(p_file.get_size());
    f->setInitPercentComplete(p_file.get_init_percent_complete());
    return f;
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

static std::shared_ptr<file>
from_swagger(const BlockFile& p_file)
{
    auto f = std::make_shared<file>();
    f->set_id(p_file.getId());
    f->set_path(p_file.getPath());
    f->set_state(p_file.getState());
    f->set_size(p_file.getFileSize());
    f->set_init_percent_complete(p_file.getInitPercentComplete());
    return f;
}

static std::shared_ptr<block_generator>
from_swagger(const BlockGenerator& p_generator)
{
    auto generator = std::make_shared<block_generator>();
    generator->set_id(p_generator.getId());
    generator->set_resource_id(p_generator.getResourceId());
    generator->set_running(p_generator.isRunning());

    block_generator_config config;
    config.pattern = block_generation_pattern_from_string(p_generator.getConfig()->getPattern());
    config.queue_depth = p_generator.getConfig()->getQueueDepth();
    config.read_size = p_generator.getConfig()->getReadSize();
    config.reads_per_sec = p_generator.getConfig()->getReadsPerSec();
    config.write_size = p_generator.getConfig()->getWriteSize();
    config.writes_per_sec = p_generator.getConfig()->getWritesPerSec();

    generator->set_config(config);

    return generator;
}


#endif // _OP_BLOCK_UTILS_HPP_