#include <cstring>
#include <stdexcept>
#include "block_generator.hpp"
#include "file_stack.hpp"
#include "device_stack.hpp"

namespace openperf::block::generator {

block_generator::block_generator(
    const model::block_generator& generator_model,
    const std::vector<virtual_device_stack*> vdev_stack_list)
    : model::block_generator(generator_model)
    , m_vdev_stack_list(vdev_stack_list)
{
    update_resource(get_resource_id());
    auto config = generate_worker_config(get_config());
    m_worker = std::make_unique<block_worker>(config);
    m_worker->start();
    if (is_running()) m_worker->resume();
}

block_generator::~block_generator()
{
    m_worker->stop();
    m_vdev->vclose();
}

block_result_ptr block_generator::start() {
    set_running(true);
    return get_statistics();
}

void block_generator::stop() { set_running(false); }

void block_generator::set_config(const model::block_generator_config& value)
{
    m_worker->config(generate_worker_config(value));
    model::block_generator::set_config(value);
}

void block_generator::set_resource_id(const std::string& value)
{
    auto dev = m_vdev;
    update_resource(value);
    if (dev) m_vdev->vclose();
    m_worker->config(generate_worker_config(get_config()));
    model::block_generator::set_resource_id(value);
}

void block_generator::update_resource(const std::string& resource_id)
{
    std::shared_ptr<virtual_device> vdev_ptr;
    for (auto vdev_stack : m_vdev_stack_list) {
        if (auto vdev = vdev_stack->get_vdev(resource_id)) {
            vdev_ptr = vdev;
            break;
        }
    }
    if (!vdev_ptr)
        throw std::runtime_error("Unknown resource: " + resource_id);

    if (auto result = vdev_ptr->vopen(); !result)
        throw std::runtime_error("Cannot open resource: "
                                 + std::string(std::strerror(result.error())));

    m_vdev = vdev_ptr;
}

void block_generator::set_running(bool is_running)
{
    if (is_running)
        m_worker->resume();
    else
        m_worker->pause();

    model::block_generator::set_running(is_running);
}

block_result_ptr block_generator::get_statistics() const
{
    auto worker_stat = m_worker->stat();

    auto generate_gen_stat = [](const task_operation_stat_t& task_stat) {
        return model::block_generator_statistics{
            .bytes_actual = static_cast<int64_t>(task_stat.bytes_actual),
            .bytes_target = static_cast<int64_t>(task_stat.bytes_target),
            .io_errors = static_cast<int64_t>(task_stat.errors),
            .ops_actual = static_cast<int64_t>(task_stat.ops_actual),
            .ops_target = static_cast<int64_t>(task_stat.ops_target),
            .latency = task_stat.latency,
            .latency_min = task_stat.latency_min,
            .latency_max = task_stat.latency_max};
    };

    auto gen_stat = std::make_shared<model::block_generator_result>();
    gen_stat->set_id(worker_stat.id);
    gen_stat->set_generator_id(get_id());
    gen_stat->set_active(is_running());
    gen_stat->set_read_stats(generate_gen_stat(worker_stat.read));
    gen_stat->set_write_stats(generate_gen_stat(worker_stat.write));
    gen_stat->set_timestamp(worker_stat.updated);
    return gen_stat;
}

void block_generator::clear_statistics() { m_worker->clear_stat(); }

task_config_t block_generator::generate_worker_config(
    const model::block_generator_config& p_config)
{
    auto fd = m_vdev->get_fd();
    assert(fd);

    task_config_t w_config;
    w_config.queue_depth = p_config.queue_depth;
    w_config.read_size = p_config.read_size;
    w_config.reads_per_sec = p_config.reads_per_sec;
    w_config.write_size = p_config.write_size;
    w_config.writes_per_sec = p_config.writes_per_sec;
    w_config.pattern = p_config.pattern;
    w_config.fd = fd.value();
    w_config.f_size = m_vdev->get_size();
    w_config.header_size = m_vdev->get_header_size();
    return w_config;
}

} // namespace openperf::block::generator