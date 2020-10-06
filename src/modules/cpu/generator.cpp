#include <future>

#include "generator.hpp"
#include "cpu.hpp"
#include "task_cpu.hpp"
#include "task_cpu_system.hpp"

#include "config/op_config_file.hpp"
#include "framework/core/op_uuid.hpp"
#include "framework/core/op_cpuset.h"

namespace openperf::cpu::generator {

static uint16_t serial_counter = 0;
constexpr auto NAME_PREFIX = "op_cpu";

std::optional<double> get_field(const cpu_stat& stat, std::string_view name)
{
    if (name == "timestamp") return stat.available.count();

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
    }

    return std::nullopt;
}

// Constructors & Destructor
generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , m_result_id(core::to_string(core::uuid::random()))
    , m_serial_number(++serial_counter)
    , m_stat_ptr(&m_stat)
    , m_dynamic(get_field)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
{
    generator::config(generator_model.config());
    m_controller.start<task_cpu_stat*>([this](const task_cpu_stat& stat) {
        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;
        m_stat += stat;

        if (m_config.utilization) {
            auto process_stat = internal::cpu_process_time();
            m_stat.utilization = process_stat.utilization;
            m_stat.system = process_stat.system;
            m_stat.user = process_stat.user;
            m_stat.steal = internal::cpu_steal_time();
        } else {
            if (m_stat.steal == 0ns) m_stat.steal = internal::cpu_steal_time();
        }

        m_dynamic.add(m_stat);
        m_stat_ptr = &m_stat;
    });
}

generator::~generator()
{
    stop();
    m_controller.clear();
}

// Methods : public
void generator::config(const generator_config& config)
{
    m_controller.pause();

    // It is required to change thread affinity in order to be able to detect
    // available cores. Affinity of the current thread could be already set and
    // we should not touch it therefore this will be performed in a separate
    // thread.
    auto available_cores = std::async(op_get_cpu_online).get();
    OP_LOG(OP_LOG_DEBUG, "Detected %zu cores", available_cores.size());

    auto mask = openperf::config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
        "modules.cpu.cpu-mask");
    if (mask) {
        std::vector<uint64_t> filtered_cores;
        for (size_t i = 0; i < available_cores.size(); i++) {
            if (mask.value() & (1 << i)) {
                filtered_cores.push_back(available_cores.at(i));
            }
        }
        OP_LOG(OP_LOG_DEBUG, "Filtered %zu cores", filtered_cores.size());
        available_cores.clear();
        std::move(filtered_cores.begin(),
                  filtered_cores.end(),
                  std::back_inserter(available_cores));
    }

    m_config = config;
    if (m_config.utilization) {
        auto sys_util = m_config.utilization.value();
        if (sys_util < 0 || 100 * available_cores.size() < sys_util) {
            throw std::runtime_error(
                "System utilization value " + std::to_string(sys_util)
                + " is not valid. Available values from 0 to "
                + std::to_string(100 * available_cores.size()));
        }

        m_stat = {available_cores.size()};
        for (uint16_t core = 0; core < available_cores.size(); ++core) {
            auto task =
                internal::task_cpu_system(core,
                                          available_cores.size(),
                                          sys_util / available_cores.size());

            m_config.cores.push_back(task.config());
            m_stat.cores[core].targets.resize(task.config().targets.size());
            m_controller.add(std::move(task),
                             NAME_PREFIX + std::to_string(m_serial_number)
                                 + "_c"
                                 + std::to_string(available_cores.at(core)),
                             available_cores.at(core));
        }
    } else {
        if (m_config.cores.size() > available_cores.size()) {
            throw std::runtime_error(
                "Could not configure more cores than available ("
                + std::to_string(available_cores.size()) + ").");
        }

        m_stat = {m_config.cores.size()};
        for (size_t core = 0; core < m_config.cores.size(); ++core) {
            auto core_conf = m_config.cores.at(core);
            for (const auto& target : core_conf.targets) {
                if (!available(target.set) || !enabled(target.set)) {
                    throw std::runtime_error(
                        "Instruction set " + std::string(to_string(target.set))
                        + " is not supported");
                }
            }

            m_stat.cores[core].targets.resize(core_conf.targets.size());

            if (core_conf.utilization == 0.0) continue;
            if (core_conf.utilization < 0.0 || 100.0 < core_conf.utilization) {
                throw std::runtime_error("Core utilization value "
                                         + std::to_string(core_conf.utilization)
                                         + " is not valid. Available values "
                                           "from 0 to 100 inclusive.");
            }

            core_conf.core = core;
            auto task = internal::task_cpu{core_conf};
            m_controller.add(std::move(task),
                             NAME_PREFIX + std::to_string(m_serial_number)
                                 + "_c"
                                 + std::to_string(available_cores.at(core)),
                             available_cores.at(core));
        }
    }

    if (m_running) m_controller.resume();
}

model::generator_result generator::statistics() const
{
    auto stat = model::generator_result{};
    stat.id(m_result_id);
    stat.generator_id(m_id);
    stat.active(m_running);
    stat.timestamp(timesync::chrono::realtime::now());
    stat.stats(*m_stat_ptr);
    stat.dynamic_results(m_dynamic.result());

    return stat;
}

void generator::start(const dynamic::configuration& cfg)
{
    if (m_running) return;

    m_dynamic.configure(cfg, m_stat);
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
    m_stat.clear();

    m_result_id = core::to_string(core::uuid::random());

    if (m_running) m_controller.resume();
}

} // namespace openperf::cpu::generator
