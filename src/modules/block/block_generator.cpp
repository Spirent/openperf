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
    close_resource(get_resource_id());
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
    close_resource(get_resource_id());
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

void block_generator::close_resource(const std::string& resource_id)
{
    if (auto blk_file = block::file::file_stack::instance().get_block_file(resource_id); blk_file) {
        blk_file->vclose();
    } else
    if (auto blk_dev = block::device::device_stack::instance().get_block_device(resource_id); blk_dev) {
        blk_dev->vclose();
    }
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

    model::block_generator::set_running(value);
}

block_result_ptr block_generator::get_statistics() const
{
    auto worker_stat = blkworker->stat();

    auto generate_gen_stat = [](const task_operation_stat_t& task_stat){
        model::block_generator_statistics gen_stat;
        gen_stat.bytes_actual = task_stat.bytes_actual;
        gen_stat.bytes_target = task_stat.bytes_target;
        gen_stat.io_errors = task_stat.errors;
        gen_stat.ops_actual = task_stat.ops_actual;
        gen_stat.ops_target = task_stat.ops_target;
        gen_stat.latency = task_stat.latency;
        gen_stat.latency_min = task_stat.latency_min;
        gen_stat.latency_max = task_stat.latency_max;
        return gen_stat;
    };

    auto gen_stat = std::make_shared<model::block_generator_result>();
    gen_stat->set_id(get_id());
    gen_stat->set_active(is_running());
    gen_stat->set_read_stats(generate_gen_stat(worker_stat.read));
    gen_stat->set_write_stats(generate_gen_stat(worker_stat.write));
    gen_stat->set_timestamp(worker_stat.updated);
    return gen_stat;
}

void block_generator::clear_statistics()
{
    blkworker->clear_stat();
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