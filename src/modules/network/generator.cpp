#include <future>

#include "generator.hpp"
#include "config/op_config_file.hpp"

namespace openperf::network::internal {

static uint16_t serial_counter = 0;
constexpr auto NAME_PREFIX = "op_network";

std::optional<double> get_field(const model::generator_result& stat,
                                std::string_view name)
{
    /*if (name == "timestamp") return stat.available.count();

    if (name == "available") return stat.available.count();
    if (name == "utilization") return stat.utilization.count();
    if (name == "system") return stat.system.count();
    if (name == "user") return stat.user.count();
    if (name == "steal") return stat.steal.count();
    if (name == "error") return stat.error.count();

    // Parse the cores[N] field name
    constexpr auto prefix = "cores[";
    auto prefix_size = std::strlen(prefix);
    if (name.substr(0, prefix_size) == prefix) {
        auto core_number = std::stoul(std::string(name.substr(
            prefix_size, name.find_first_of(']', prefix_size) - prefix_size)));

        if (core_number < stat.cores.size()) {
            auto field_name =
                name.substr(name.find_first_of('.', prefix_size) + 1);

            if (field_name == "available")
                return stat.cores[core_number].available.count();
            if (field_name == "utilization")
                return stat.cores[core_number].utilization.count();
            if (field_name == "system")
                return stat.cores[core_number].system.count();
            if (field_name == "user")
                return stat.cores[core_number].user.count();
            if (field_name == "steal")
                return stat.cores[core_number].steal.count();
            if (field_name == "error")
                return stat.cores[core_number].error.count();

            // Parse the targest[N] field name
            constexpr auto target_prefix = "targets[";
            auto target_prefix_size = std::strlen(target_prefix);
            if (field_name.substr(0, target_prefix_size) == target_prefix) {
                auto target_number = std::stoul(std::string(field_name.substr(
                    target_prefix_size,
                    field_name.find_first_of(']', target_prefix_size)
                        - target_prefix_size)));

                if (target_number < stat.cores[core_number].targets.size()) {
                    auto target_field = field_name.substr(
                        field_name.find_first_of('.', target_prefix_size) + 1);

                    if (target_field == "operations")
                        return stat.cores[core_number]
                            .targets[target_number]
                            .operations;
                }
            }
        }
    }*/

    return std::nullopt;
}

// Constructors & Destructor
generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , m_result_id(core::to_string(core::uuid::random()))
    , m_serial_number(++serial_counter)
    , m_read_stat_ptr(&m_read_stat)
    , m_write_stat_ptr(&m_write_stat)
    , m_dynamic(get_field)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
{
    generator::config(generator_model.config());
    // m_controller.start<model::generator_result::statistics_t*>([this](const
    // task_cpu_stat& stat) {
    //     auto stat_copy = m_stat;
    //     m_stat_ptr = &stat_copy;
    //     m_stat += stat;

    //     if (m_stat.steal == 0ns) m_stat.steal = internal::cpu_steal_time();

    //     m_dynamic.add(m_stat);
    //     m_stat_ptr = &m_stat;
    // });
}

generator::~generator()
{
    stop();
    m_controller.clear();
}

void generator::config(const model::generator_config& config)
{
    m_controller.pause();

    if (m_running) m_controller.resume();
}

model::generator_result generator::statistics() const
{
    auto stat = model::generator_result{};
    stat.id(m_result_id);
    stat.generator_id(m_id);
    stat.active(m_running);
    stat.timestamp(timesync::chrono::realtime::now());
    stat.start_timestamp(m_start_time);
    stat.read_stats(*m_read_stat_ptr);
    stat.write_stats(*m_write_stat_ptr);
    stat.dynamic_results(m_dynamic.result());

    return stat;
}

void generator::start(const dynamic::configuration& cfg)
{
    if (m_running) return;

    m_dynamic.configure(cfg);
    start();
}

void generator::start()
{
    if (m_running) return;

    reset();
    m_controller.resume();
    m_running = true;
    m_dynamic.reset();
}

void generator::stop()
{
    m_controller.pause();
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
    m_controller.pause();
    m_controller.reset();
    m_read_stat = {};
    m_write_stat = {};

    m_result_id = core::to_string(core::uuid::random());
    m_start_time = chronometer::now();

    if (m_running) m_controller.resume();
}

} // namespace openperf::network::internal
