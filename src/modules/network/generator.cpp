#include <future>

#include "config/op_config_file.hpp"
#include "arg_parser.hpp"
#include "drivers/kernel.hpp"
#include "drivers/dpdk.hpp"
#include "generator.hpp"

namespace openperf::network::internal {

using namespace std::chrono_literals;

static uint16_t serial_counter = 0;
constexpr auto NAME_PREFIX = "op_network";

auto to_load_stat_t(const task::stat_t& task_stat)
{
    return model::generator_result::load_stat_t{
        .ops_target = task_stat.ops_target,
        .ops_actual = task_stat.ops_actual,
        .bytes_target = task_stat.bytes_target,
        .bytes_actual = task_stat.bytes_actual,
        .io_errors = task_stat.errors,
        .latency = task_stat.latency,
        .latency_min = task_stat.latency_min,
        .latency_max = task_stat.latency_max};
};

auto to_conn_stat_t(const task::stat_t& write_stat,
                    const task::stat_t& read_stat)
{
    return model::generator_result::conn_stat_t{
        .attempted =
            read_stat.conn_stat.attempted + write_stat.conn_stat.attempted,
        .successful =
            read_stat.conn_stat.successful + write_stat.conn_stat.successful,
        .closed = read_stat.conn_stat.closed + write_stat.conn_stat.closed,
        .errors = read_stat.conn_stat.errors + write_stat.conn_stat.errors,
    };
};

std::optional<double> get_field(const model::generator_result& stat,
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

inline int to_task_protocol(model::protocol_t p)
{
    switch (p) {
    case model::TCP:
        return (IPPROTO_TCP);
    case model::UDP:
        return (IPPROTO_UDP);
    default:
        return (IPPROTO_IP); /* a.k.a. 0 */
    }
}

stat_collector::stat_collector(generator* gen)
    : m_gen(gen)
{}

void stat_collector::set_generator(generator* gen) { m_gen = gen; }

void stat_collector::update(const task::stat_t& stat)
{
    auto elapsed_time = stat.updated - m_gen->m_start_time;
    auto& read_stat = m_gen->m_read_stat;
    auto& write_stat = m_gen->m_write_stat;
    auto& config = m_gen->m_config;

    switch (stat.operation) {
    case task::operation_t::READ:
        read_stat += stat;
        using double_time = std::chrono::duration<double>;
        // +1 here is because target value has to be rounded to the upper
        // value, otherwise it will be equal 0 on a first iteration
        read_stat.ops_target = static_cast<uint64_t>(
            (std::chrono::duration_cast<double_time>(elapsed_time)
             * std::min(config.read_size, 1UL) * config.reads_per_sec)
                .count()
            + 1);
        read_stat.bytes_target = read_stat.ops_target * config.read_size;
        break;
    case task::operation_t::WRITE:
        write_stat += stat;
        // +1 here is because target value has to be rounded to the upper
        // value, otherwise it will be equal 0 on a first iteration
        write_stat.ops_target = static_cast<uint64_t>(
            (std::chrono::duration_cast<double_time>(elapsed_time)
             * std::min(config.write_size, 1UL) * config.writes_per_sec)
                .count()
            + 1);
        write_stat.bytes_target = write_stat.ops_target * config.write_size;
        break;
    }

    auto complete_stat = model::generator_result();
    complete_stat.read_stats(to_load_stat_t(read_stat));
    complete_stat.write_stats(to_load_stat_t(write_stat));
    complete_stat.conn_stats(to_conn_stat_t(write_stat, read_stat));
    complete_stat.timestamp(std::max(write_stat.updated, read_stat.updated));

    m_gen->m_dynamic.add(complete_stat);
}

// Constructors & Destructor
generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , m_result_id(core::to_string(core::uuid::random()))
    , m_serial_number(++serial_counter)
    , m_dynamic(get_field)
{
    m_stat_collector = std::make_unique<stat_collector>(this);
    generator::config(m_config);
}

generator::~generator()
{
    stop();

    if (m_controller) m_controller->clear();
}

void generator::config(const model::generator_config& config)
{
    auto nd = config::driver();

    if (!m_target.interface && nd->bind_to_device_required())
        throw std::runtime_error(
            "Bind to the interface is required for this driver");

    if (!m_controller) {
        return; // just do validation if controller is not running
    }

    if (m_running) m_controller->pause();

    auto t_config =
        task::config_t{.operation = task::operation_t::READ,
                       .connections = m_config.connections,
                       .ops_per_connection = m_config.ops_per_connection,
                       .target = task::target_t{
                           .host = m_target.host,
                           .port = m_target.port,
                           .protocol = to_task_protocol(m_target.protocol),
                           .interface = m_target.interface,
                       }};

    if (config.ratio) {
        t_config.synchronizer = &m_synchronizer;
        m_synchronizer.ratio_reads.store(config.ratio.value().reads,
                                         std::memory_order_relaxed);
        m_synchronizer.ratio_writes.store(config.ratio.value().writes,
                                          std::memory_order_relaxed);
    }

    // Find existing read/write tasks
    task::network_task *read_task = nullptr, *write_task = nullptr;
    for (auto bt : m_controller->get_tasks()) {
        auto* task = static_cast<task::network_task*>(bt);
        switch (task->config().operation) {
        case task::operation_t::READ:
            read_task = task;
            break;
        case task::operation_t::WRITE:
            write_task = task;
            break;
        }
    }

    if (config.reads_per_sec > 0) {
        t_config.operation = task::operation_t::READ;
        t_config.block_size = m_config.read_size;
        t_config.ops_per_sec = m_config.reads_per_sec;
        if (!read_task) {
            auto task = std::make_unique<task::network_task>(t_config, nd);
            m_controller->add(
                task, NAME_PREFIX + std::to_string(m_serial_number) + "r");
        } else {
            read_task->config(t_config);
        }
    } else {
        m_controller->remove_task(read_task);
    }

    if (config.writes_per_sec > 0) {
        t_config.operation = task::operation_t::WRITE;
        t_config.block_size = m_config.write_size;
        t_config.ops_per_sec = m_config.writes_per_sec;
        if (!write_task) {
            auto task = std::make_unique<task::network_task>(t_config, nd);
            m_controller->add(
                task, NAME_PREFIX + std::to_string(m_serial_number) + "w");
        } else {
            write_task->config(t_config);
        }
    } else {
        m_controller->remove_task(write_task);
    }

    if (m_running) m_controller->resume();
}

model::generator_result generator::statistics() const
{
    auto stat = model::generator_result{};
    stat.id(m_result_id);
    stat.generator_id(m_id);
    stat.active(m_running);
    stat.timestamp(timesync::chrono::realtime::now());
    stat.start_timestamp(m_start_time);
    stat.read_stats(to_load_stat_t(m_read_stat));
    stat.write_stats(to_load_stat_t(m_write_stat));
    stat.conn_stats(to_conn_stat_t(m_write_stat, m_read_stat));
    stat.dynamic_results(m_dynamic.result());

    return stat;
}

void generator::start(const dynamic::configuration& cfg)
{
    if (m_running) return;

    m_dynamic.configure(cfg);
    start();
}

void swap_running(generator& out_gen,
                  generator& in_gen,
                  const dynamic::configuration& cfg)
{
    bool in_running = out_gen.running();
    out_gen.stop();

    // The serial number should go with the controller so the thread names match
    std::swap(in_gen.m_serial_number, out_gen.m_serial_number);

    // The stat_collector is used by the controller stats maintenance thread so
    // need to swap it and update the generator it references.
    std::swap(in_gen.m_stat_collector, out_gen.m_stat_collector);
    in_gen.m_stat_collector->set_generator(&in_gen);
    out_gen.m_stat_collector->set_generator(&out_gen);

    // Swap the controller between the generators and update the task
    // configuration to the new generator.
    std::swap(in_gen.m_controller, out_gen.m_controller);
    in_gen.config(in_gen.m_config);

    if (in_running) in_gen.start(cfg);
}

void generator::start()
{
    if (m_running) return;

    if (!m_controller) {
        m_controller = std::make_unique<controller>(
            NAME_PREFIX + std::to_string(m_serial_number) + "_ctl");
        m_controller->start<task::stat_t>(
            [stat_collector = m_stat_collector.get()](
                const task::stat_t& stat) { stat_collector->update(stat); });

        generator::config(m_config);
    }

    reset();
    m_controller->resume();
    m_running = true;
    m_dynamic.reset();
}

void generator::stop()
{
    if (!m_running) return;

    m_controller->pause();
    m_running = false;
}

void generator::running(bool is_running)
{
    if (is_running)
        start();
    else
        stop();
}

void generator::reset()
{
    if (!m_controller) return;

    m_controller->pause();
    m_controller->reset();
    m_read_stat = {.operation = task::operation_t::READ};
    m_write_stat = {.operation = task::operation_t::WRITE};

    m_result_id = core::to_string(core::uuid::random());
    m_start_time = chronometer::now();

    if (m_running) m_controller->resume();
}

} // namespace openperf::network::internal
