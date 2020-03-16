#ifndef _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_
#define _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_

#include "GeneratorConfig.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"

namespace openperf::memory {

namespace swagger = ::swagger::v1::model;

GeneratorConfig swaggerModelToConfig(const model::MemoryGeneratorConfig& m)
{
    return GeneratorConfig::create()
        .setBufferSize(m.getBufferSize())
        .setReadSize(m.getReadSize())
        .setReadsPerSec(m.getReadsPerSec())
        .setReadThreads(m.getReadThreads())
        .setWriteSize(m.getWriteSize())
        .setWritesPerSec(m.getWritesPerSec())
        .setWriteThreads(m.getWriteThreads());
}

swagger::MemoryGeneratorConfig
configToSwaggerModel(const GeneratorConfig& config)
{
    swagger::MemoryGeneratorConfig config_model;
    config_model.setBufferSize(config.bufferSize());
    config_model.setReadSize(config.readSize());
    config_model.setReadsPerSec(config.readsPerSec());
    config_model.setReadThreads(config.readThreads());
    config_model.setWriteSize(config.writeSize());
    config_model.setWritesPerSec(config.writesPerSec());
    config_model.setWriteThreads(config.writeThreads());

    return config_model;
}

} // namespace openperf::memory

#endif // _OP_MEMORY_GENERATORCONFIGCONVERTERS_HPP_
