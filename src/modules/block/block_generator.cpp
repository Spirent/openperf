#include "block_generator.hpp"

namespace openperf::block::generator {

block_generator::block_generator(const model::block_generator& generator_model)
    : model::block_generator(generator_model)
{
    auto config = generate_worker_config(get_config());
    blkworker = block_worker_ptr(new block_worker(config, is_running(), get_config().pattern));
}

void block_generator::start() 
{
    set_running(true);
}

void block_generator::stop()
{
    set_running(false);
}

void block_generator::set_сonfig(const model::block_generator_config& value)
{
    model::block_generator::set_config(value);
}

void block_generator::set_resource_id(const std::string& value)
{
    model::block_generator::set_resource_id(value);
}

void block_generator::set_running(bool value)
{
    model::block_generator::set_running(value);
    blkworker->set_running(value);
}

worker_config block_generator::generate_worker_config(const model::block_generator_config& generator_config) {
    worker_config w_config;
    w_config.queue_depth = generator_config.queue_depth;
    w_config.read_size = generator_config.read_size;
    w_config.reads_per_sec = generator_config.reads_per_sec;
    w_config.write_size = generator_config.write_size;
    w_config.writes_per_sec = generator_config.writes_per_sec;
    return w_config;
}

} // namespace openperf::block::generator