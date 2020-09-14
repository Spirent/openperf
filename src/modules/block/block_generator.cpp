#include <cstring>
#include <stdexcept>

#include "block_generator.hpp"
#include "device_stack.hpp"
#include "file_stack.hpp"

namespace openperf::block::generator {

using namespace std::chrono_literals;

static uint16_t serial_counter = 0;

auto to_statistics_t(const task_stat_t& task_stat)
{
    return model::block_generator_result::statistics_t{
        .bytes_actual = task_stat.bytes_actual,
        .bytes_target = task_stat.bytes_target,
        .io_errors = task_stat.errors,
        .ops_actual = task_stat.ops_actual,
        .ops_target = task_stat.ops_target,
        .latency = task_stat.latency,
        .latency_min = task_stat.latency_min,
        .latency_max = task_stat.latency_max};
};

std::optional<double> get_field(const model::block_generator_result& stat,
                                std::string_view name)
{
    auto read = stat.read_stats();
    if (name == "read.ops_target") return read.ops_target;
    if (name == "read.ops_actual") return read.ops_actual;
    if (name == "read.bytes_target") return read.bytes_target;
    if (name == "read.bytes_actual") return read.bytes_actual;
    if (name == "read.io_errors") return read.io_errors;
    if (name == "read.latency_total") return read.latency.count();

    if (name == "read.latency_min")
        return read.latency_min.value_or(0ns).count();
    if (name == "read.latency_max")
        return read.latency_max.value_or(0ns).count();

    auto write = stat.write_stats();
    if (name == "write.ops_target") return write.ops_target;
    if (name == "write.ops_actual") return write.ops_actual;
    if (name == "write.bytes_target") return write.bytes_target;
    if (name == "write.bytes_actual") return write.bytes_actual;
    if (name == "write.io_errors") return write.io_errors;
    if (name == "write.latency_total") return write.latency.count();

    if (name == "write.latency_min")
        return write.latency_min.value_or(0ns).count();
    if (name == "write.latency_max")
        return write.latency_max.value_or(0ns).count();

    if (name == "timestamp") return stat.timestamp().time_since_epoch().count();

    return std::nullopt;
}

block_generator::block_generator(
    const model::block_generator& generator_model,
    const std::vector<virtual_device_stack*>& vdev_stack_list)
    : model::block_generator(generator_model)
    , m_serial_number(++serial_counter)
    , m_vdev_stack_list(vdev_stack_list)
    , m_dynamic(get_field)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
{
    m_controller.start<task_stat_t>([this](const task_stat_t& stat) {
        auto elapsed_time = stat.updated - m_start_time;

        switch (stat.operation) {
        case task_operation::READ:
            m_read += stat;
            m_read.ops_target = elapsed_time.count() * m_config.reads_per_sec
                                * std::min(m_config.read_size, 1U)
                                / std::nano::den;
            m_read.bytes_target = m_read.ops_target * m_config.read_size;
            break;
        case task_operation::WRITE:
            m_write += stat;
            m_write.ops_target = elapsed_time.count() * m_config.writes_per_sec
                                 * std::min(m_config.write_size, 1U)
                                 / std::nano::den;
            m_write.bytes_target = m_write.ops_target * m_config.write_size;
            break;
        }

        auto complete_stat = model::block_generator_result();
        complete_stat.read_stats(to_statistics_t(m_read));
        complete_stat.write_stats(to_statistics_t(m_write));
        complete_stat.timestamp(std::max(m_write.updated, m_read.updated));
        complete_stat.dynamic_results(m_dynamic.result());

        m_dynamic.add(complete_stat);
    });

    update_resource(m_resource_id);
    block_generator::config(m_config);
}

block_generator::~block_generator()
{
    m_controller.clear();
    m_vdev->close();
}

block_generator::block_result_ptr block_generator::start()
{
    if (m_running) return statistics();

    reset();
    m_controller.resume();
    m_running = true;
    return statistics();
}

block_generator::block_result_ptr
block_generator::start(const dynamic::configuration& config)
{
    if (m_running) return statistics();

    m_dynamic.configure(config);
    return start();
}

void block_generator::stop()
{
    if (!m_running) return;

    m_controller.pause();
    m_running = false;
}

void block_generator::config(const model::block_generator_config& value)
{
    m_controller.pause();

    m_controller.clear();
    reset();

    if (value.reads_per_sec && value.read_size) {
        m_controller.add(
            block_task{make_task_config(value, task_operation::READ)},
            NAME_PREFIX + std::to_string(m_serial_number) + "_read");
    }

    if (value.writes_per_sec && value.write_size) {
        m_controller.add(
            block_task{make_task_config(value, task_operation::WRITE)},
            NAME_PREFIX + std::to_string(m_serial_number) + "_write");
    }

    if (m_running) m_controller.resume();
}

void block_generator::resource_id(std::string_view value)
{
    auto dev = m_vdev;
    update_resource(std::string(value));
    if (dev) m_vdev->close();

    config(m_config);
    m_resource_id = value;
}

void block_generator::update_resource(const std::string& resource_id)
{
    std::shared_ptr<virtual_device> vdev_ptr;
    for (auto vdev_stack : m_vdev_stack_list) {
        if (auto vdev = vdev_stack->vdev(resource_id)) {
            vdev_ptr = vdev;
            break;
        }
    }

    if (!vdev_ptr)
        throw std::runtime_error("Unknown or unusable resource: "
                                 + resource_id);

    if (auto result = vdev_ptr->open(); !result)
        throw std::runtime_error("Cannot open resource: "
                                 + std::string(std::strerror(result.error())));

    m_vdev = vdev_ptr;
}

void block_generator::running(bool is_running)
{
    if (is_running)
        start();
    else
        stop();
}

block_generator::block_result_ptr block_generator::statistics() const
{
    auto stat = std::make_shared<model::block_generator_result>();
    stat->id(m_statistics_id);
    stat->generator_id(m_id);
    stat->active(m_running);
    stat->read_stats(to_statistics_t(m_read));
    stat->write_stats(to_statistics_t(m_write));
    stat->timestamp(std::max(m_write.updated, m_read.updated));
    stat->dynamic_results(m_dynamic.result());

    return stat;
}

void block_generator::reset()
{
    m_controller.pause();
    m_controller.reset();
    m_dynamic.reset();

    m_read = {.operation = task_operation::READ};
    m_write = {.operation = task_operation::WRITE};
    m_statistics_id = core::to_string(core::uuid::random());
    m_start_time = chronometer::now();

    if (m_running) m_controller.resume();
}

task_config_t
block_generator::make_task_config(const model::block_generator_config& config,
                                  task_operation op)
{
    auto block_size =
        (op == task_operation::READ) ? config.read_size : config.write_size;

    if (static_cast<uint64_t>(block_size) + m_vdev->header_size()
        > m_vdev->size())
        throw std::runtime_error(
            "Cannot use resource: resource size is too small");

    auto fd = m_vdev->fd();
    assert(fd);

    task_config_t t_config{
        .fd = (op == task_operation::READ) ? fd.value().read : fd.value().write,
        .f_size = m_vdev->size(),
        .header_size = m_vdev->header_size(),
        .queue_depth = config.queue_depth,
        .operation = op,
        .ops_per_sec = (op == task_operation::READ) ? config.reads_per_sec
                                                    : config.writes_per_sec,
        .block_size = block_size,
        .pattern = config.pattern,
    };

    if (config.ratio) {
        t_config.synchronizer = &m_synchronizer;
        m_synchronizer.ratio_reads.store(config.ratio.value().reads,
                                         std::memory_order_relaxed);
        m_synchronizer.ratio_writes.store(config.ratio.value().writes,
                                          std::memory_order_relaxed);
    }

    return t_config;
}

} // namespace openperf::block::generator
