#include <cstring>
#include <stdexcept>
#include "block_generator.hpp"
#include "file_stack.hpp"
#include "device_stack.hpp"

namespace openperf::block::generator {

block_generator::block_generator(
    const model::block_generator& generator_model,
    const std::vector<virtual_device_stack*>& vdev_stack_list)
    : model::block_generator(generator_model)
    , m_statistics_id(core::to_string(core::uuid::random()))
    , m_vdev_stack_list(vdev_stack_list)
{
    update_resource(m_resource_id);

    auto read_config = generate_worker_config(m_config, task_operation::READ);
    m_read_worker = std::make_unique<block_worker>(read_config);

    auto write_config = generate_worker_config(m_config, task_operation::WRITE);
    m_write_worker = std::make_unique<block_worker>(write_config);

    m_read_worker->start();
    m_write_worker->start();

    running(m_running);
}

block_generator::~block_generator()
{
    m_read_worker->stop();
    m_write_worker->stop();
    m_vdev->vclose();
}

block_result_ptr block_generator::start()
{
    running(true);
    return statistics();
}

void block_generator::stop() { running(false); }

void block_generator::config(const model::block_generator_config& value)
{
    m_read_worker->config(generate_worker_config(value, task_operation::READ));
    m_write_worker->config(
        generate_worker_config(value, task_operation::WRITE));

    m_config = value;
}

void block_generator::resource_id(std::string_view value)
{
    auto dev = m_vdev;
    update_resource(std::string(value));
    if (dev) m_vdev->vclose();

    config(m_config);

    m_resource_id = value;
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
        throw std::runtime_error("Unknown or unusable resource: "
                                 + resource_id);

    if (auto result = vdev_ptr->vopen(); !result)
        throw std::runtime_error("Cannot open resource: "
                                 + std::string(std::strerror(result.error())));

    m_vdev = vdev_ptr;
}

void block_generator::running(bool is_running)
{
    if (is_running) {
        if (m_config.reads_per_sec > 0) m_read_worker->resume();
        if (m_config.writes_per_sec > 0) m_write_worker->resume();
    } else {
        m_read_worker->pause();
        m_write_worker->pause();
    }

    m_running = is_running;
}

block_result_ptr block_generator::statistics() const
{
    auto read_stat = m_read_worker->stat();
    auto write_stat = m_write_worker->stat();

    auto generate_gen_stat = [](const task_stat_t& task_stat) {
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
    gen_stat->id(m_statistics_id);
    gen_stat->generator_id(m_id);
    gen_stat->active(m_running);
    gen_stat->read_stats(generate_gen_stat(read_stat));
    gen_stat->write_stats(generate_gen_stat(write_stat));
    gen_stat->timestamp(std::max(write_stat.updated, read_stat.updated));
    return gen_stat;
}

void block_generator::clear_statistics()
{
    m_read_worker->clear_stat();
    m_write_worker->clear_stat();
    m_statistics_id = core::to_string(core::uuid::random());
}

task_config_t block_generator::generate_worker_config(
    const model::block_generator_config& p_config, task_operation p_operation)
{
    auto block_size = (p_operation == task_operation::READ)
                          ? p_config.read_size
                          : p_config.write_size;
    if (static_cast<uint64_t>(block_size) + m_vdev->get_header_size()
        > m_vdev->get_size())
        throw std::runtime_error(
            "Cannot use resource: resource size is too small");

    auto fd = m_vdev->get_fd();
    assert(fd);

    task_config_t w_config;
    w_config.queue_depth = p_config.queue_depth;
    w_config.block_size = block_size;
    w_config.ops_per_sec = (p_operation == task_operation::READ)
                               ? p_config.reads_per_sec
                               : p_config.writes_per_sec;
    w_config.operation = p_operation;
    w_config.pattern = p_config.pattern;
    w_config.fd = (p_operation == task_operation::READ) ? fd.value().read
                                                        : fd.value().write;
    w_config.f_size = m_vdev->get_size();
    w_config.header_size = m_vdev->get_header_size();
    if (p_config.ratio) {
        w_config.synchronizer = &m_synchronizer;
        m_synchronizer.ratio_reads.store(p_config.ratio.value().reads,
                                         std::memory_order_relaxed);
        m_synchronizer.ratio_writes.store(p_config.ratio.value().writes,
                                          std::memory_order_relaxed);
    }

    return w_config;
}

} // namespace openperf::block::generator
