#include "generator.hpp"

namespace openperf::block::generator {

block_generator::block_generator(json& val)
{
    fromJson(val);
    
    blkworker = block_worker_ptr(new block_worker());
}

void block_generator::fromJson(json& val)
{
    BlockGenerator::fromJson(val);

    BlockGeneratorConfig config;
    config.fromJson(val["config"]);
    setConfig(std::make_shared<BlockGeneratorConfig>(config));
}

} // namespace openperf::block::generator