#include <cstring>
#include <stdexcept>
#include "block_generator.hpp"
#include "file_stack.hpp"
#include "device_stack.hpp"

namespace openperf::block::generator {

block_generator::block_generator(const model::block_generator& generator_model, const std::vector<virtual_device_stack*> vdev_stack_list)
    : model::block_generator(generator_model)
    , vdev_stack_list(vdev_stack_list)
{
    update_resource(get_resource_id());
    auto config = generate_worker_config(get_config());
    blkworker = block_worker_ptr(new block_worker(config));
    blkworker->start();
}

block_generator::~block_generator()
{
    blkworker->stop();
    blkdevice->vclose();
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
    blkworker->config(generate_worker_config(value));
    model::block_generator::set_config(value);
}

void block_generator::set_resource_id(const std::string& value)
{
    auto dev = blkdevice;
    update_resource(value);
    if (dev)
        blkdevice->vclose();
    blkworker->config(generate_worker_config(get_config()));
    model::block_generator::set_resource_id(value);
}

void block_generator::update_resource(const std::string& resource_id)
{
    std::shared_ptr<virtual_device> vdev_ptr;
    for (auto vdev_stack : vdev_stack_list) {
        if (auto vdev = vdev_stack->get_vdev(resource_id)) {
            vdev_ptr = vdev;
            break;
        }
    }
    if (!vdev_ptr)
        throw std::runtime_error("Unknown resource: " + resource_id);

    if (auto fd = vdev_ptr->vopen(); fd < 0)
        throw std::runtime_error("Cannot open resource: " + std::string(std::strerror(-fd)));

    blkdevice = vdev_ptr;
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

task_config_t block_generator::generate_worker_config(const model::block_generator_config& p_config) {
    task_config_t w_config;
    w_config.queue_depth = p_config.queue_depth;
    w_config.read_size = p_config.read_size;
    w_config.reads_per_sec = p_config.reads_per_sec;
    w_config.write_size = p_config.write_size;
    w_config.writes_per_sec = p_config.writes_per_sec;
    w_config.pattern = p_config.pattern;
    w_config.fd = blkdevice->get_fd();
    w_config.f_size = blkdevice->get_size();
    w_config.header_size = blkdevice->get_header_size();
    return w_config;
}

} // namespace openperf::block::generator