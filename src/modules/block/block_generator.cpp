#include <cstring>
#include <stdexcept>
#include "block_generator.hpp"
#include "file_stack.hpp"
#include "device_stack.hpp"

namespace openperf::block::generator {

block_generator::block_generator(const model::block_generator& generator_model)
    : model::block_generator(generator_model)
{
    auto config = generate_worker_config(get_config(), get_resource_size(get_resource_id()));
    auto fd = open_resource(get_resource_id());
    if (fd < 0)
        throw std::runtime_error("Cannot open resource: " + std::string(std::strerror(-fd)));
    blkworker = block_worker_ptr(new block_worker(config, fd, is_running(), get_config().pattern));
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
    auto fd = open_resource(value);

    blkworker->set_resource_descriptor(fd);
    model::block_generator::set_resource_id(value);

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
    } else 
    if (auto blk_dev = block::device::device_stack::instance().get_block_device(resource_id); blk_dev) {
        return blk_file->get_size();
    }
    return 0;
}

void block_generator::set_running(bool value)
{
    model::block_generator::set_running(value);
    blkworker->set_running(value);
}

worker_config block_generator::generate_worker_config(const model::block_generator_config& generator_config, const size_t& resource_size) {
    worker_config w_config;
    w_config.queue_depth = generator_config.queue_depth;
    w_config.read_size = generator_config.read_size;
    w_config.reads_per_sec = generator_config.reads_per_sec;
    w_config.write_size = generator_config.write_size;
    w_config.writes_per_sec = generator_config.writes_per_sec;
    w_config.f_size = resource_size;
    return w_config;
}

} // namespace openperf::block::generator