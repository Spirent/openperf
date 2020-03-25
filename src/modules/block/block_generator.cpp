#include <cstring>
#include <stdexcept>
#include "block_generator.hpp"
#include "file_stack.hpp"
#include "device_stack.hpp"

namespace openperf::block::generator {

block_generator::block_generator(const model::block_generator& generator_model)
    : model::block_generator(generator_model)
{
    auto config = generate_worker_config();
    blkworker = block_worker_ptr(new block_worker(config));
    blkworker->start();
}

block_generator::~block_generator()
{
    blkworker->stop();
}

void block_generator::start()
{
    set_running(true);
}

void block_generator::stop()
{
    set_running(false);
}

void block_generator::set_config(const model::block_generator_config& value)
{
    model::block_generator::set_config(value);
}

void block_generator::set_resource_id(const std::string& value)
{
    model::block_generator::set_resource_id(value);
    generate_worker_config();
}

int block_generator::open_resource(const std::string& resource_id)
{
    int fd = -1;
    if (auto blk_file = block::file::file_stack::instance().get_block_file(resource_id); blk_file) {
        fd = blk_file->vopen();
    } else
    if (auto blk_dev = block::device::device_stack::instance().get_block_device(resource_id); blk_dev) {
        fd = blk_dev->vopen();
    }
    return fd;
}

size_t block_generator::get_resource_size(const std::string& resource_id)
{
    if (auto blk_file = block::file::file_stack::instance().get_block_file(resource_id); blk_file) {
        return blk_file->get_size();
    } else if (auto blk_dev = block::device::device_stack::instance().get_block_device(resource_id); blk_dev) {
        return blk_file->get_size();
    }
    return 0;
}

void block_generator::set_running(bool value)
{
    if (value)
        blkworker->resume();
    else
        blkworker->pause();
}

task_config_t block_generator::generate_worker_config() {
    auto fd = open_resource(get_resource_id());
    if (fd < 0)
        throw std::runtime_error("Cannot open resource: " + std::string(std::strerror(-fd)));

    task_config_t w_config;
    w_config.queue_depth = get_config().queue_depth;
    w_config.read_size = get_config().read_size;
    w_config.reads_per_sec = get_config().reads_per_sec;
    w_config.write_size = get_config().write_size;
    w_config.writes_per_sec = get_config().writes_per_sec;
    w_config.pattern = get_config().pattern;
    w_config.fd = fd;
    w_config.f_size = get_resource_size(get_resource_id());
    return w_config;
}

} // namespace openperf::block::generator