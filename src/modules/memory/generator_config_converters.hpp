#ifndef _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_
#define _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_

#include "generator_config.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"

namespace openperf::memory {

namespace swagger = ::swagger::v1::model;

generator_config swagger_model_to_config(const model::MemoryGeneratorConfig& m)
{
    return generator_config::create()
        .buffer_size(m.getBufferSize())
        .read_size(m.getReadSize())
        .reads_per_sec(m.getReadsPerSec())
        .read_threads(m.getReadThreads())
        .write_size(m.getWriteSize())
        .writes_per_sec(m.getWritesPerSec())
        .write_threads(m.getWriteThreads());
}

swagger::MemoryGeneratorConfig
config_to_swagger_model(const generator_config& config)
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
