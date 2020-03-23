#ifndef _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_
#define _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_

#include "generator_config.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"

namespace openperf::memory {

namespace swagger = ::swagger::v1::model;

generator_config swaggerModelToConfig(const model::MemoryGeneratorConfig& m)
{
    return generator_config::create()
        .set_buffer_size(m.getBufferSize())
        .set_read_size(m.getReadSize())
        .set_reads_per_sec(m.getReadsPerSec())
        .set_read_threads(m.getReadThreads())
        .set_write_size(m.getWriteSize())
        .set_writes_per_sec(m.getWritesPerSec())
        .set_write_threads(m.getWriteThreads());
}

swagger::MemoryGeneratorConfig
configToSwaggerModel(const generator_config& config)
{
    swagger::MemoryGeneratorConfig config_model;
    config_model.setBufferSize(config.buffer_size());
    config_model.setReadSize(config.read_size());
    config_model.setReadsPerSec(config.reads_per_sec());
    config_model.setReadThreads(config.read_threads());
    config_model.setWriteSize(config.write_size());
    config_model.setWritesPerSec(config.writes_per_sec());
    config_model.setWriteThreads(config.write_threads());

    return config_model;
}

} // namespace openperf::memory

#endif // _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_
