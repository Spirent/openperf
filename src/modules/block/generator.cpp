#include "generator.hpp"
#include "block/workers/random.hpp"

namespace openperf::block::generator {

block_generator::block_generator(json& val)
{
    fromJson(val);

    worker_config config;
    generate_worker_config(*getConfig(), config);
    blkworker = block_worker_ptr(new block_worker(config, isRunning()));
    if (getConfig()->getPattern() == "random") {
        blkworker->add_task<worker_task_random>(new worker::worker_task_random());
    }
}

void block_generator::start() 
{
    setRunning(true);
}

void block_generator::stop()
{
    setRunning(false);
}

void block_generator::fromJson(json& val)
{
    BlockGenerator::fromJson(val);

    BlockGeneratorConfig config;
    config.fromJson(val["config"]);
    BlockGenerator::setConfig(std::make_shared<BlockGeneratorConfig>(config));
}

void block_generator::setConfig(std::shared_ptr<BlockGeneratorConfig> value)
{
    BlockGenerator::setConfig(value);
}

void block_generator::setResourceId(std::string value)
{
    BlockGenerator::setResourceId(value);
}

void block_generator::setRunning(bool value)
{
    BlockGenerator::setRunning(value);
    blkworker->set_running(value);
}

void block_generator::generate_worker_config(BlockGeneratorConfig& generator_config, worker_config &config) {
    config.pattern = generator_config.getPattern();
    config.queue_depth = generator_config.getQueueDepth();
    config.read_size = generator_config.getReadSize();
    config.reads_per_sec = generator_config.getReadsPerSec();
    config.write_size = generator_config.getWriteSize();
    config.writes_per_sec = generator_config.getWritesPerSec();
}

} // namespace openperf::block::generator