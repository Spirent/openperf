#include "generator.hpp"

#include "cpu/cpu.hpp"
#include "core/op_uuid.hpp"

namespace openperf::cpu::generator {

static uint16_t serial_counter = 0;

generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , m_result_id(core::to_string(core::uuid::random()))
    , m_serial_number(++serial_counter)
{
    configure_workers(config());
}

generator::~generator()
{
    for (auto & worker : m_workers)
        worker->stop();
}

void generator::start() {
    set_running(true);
}

void generator::stop() { set_running(false); }

void generator::set_config(const model::generator_config& p_conf)
{
    if (running())
        throw std::runtime_error(
            "Cannot change config of running generator");

    for (auto & worker : m_workers)
        worker->stop();

    configure_workers(p_conf);
    model::generator::config(p_conf);
}

void generator::set_running(bool is_running)
{
    for (auto & worker : m_workers) {
        if (is_running)
            worker->resume();
        else
            worker->pause();
    }

    model::generator::running(is_running);
}

model::generator_result generator::statistics() const
{
    model::generator_stats stats;
    for (const auto & worker : m_workers) {
        auto w_stat = worker->stat();

        stats.available += w_stat.available;
        stats.utilization += w_stat.utilization;
        stats.system += w_stat.system;
        stats.user += w_stat.user;
        stats.steal += w_stat.steal;
        stats.error += w_stat.error;

        model::generator_core_stats core_stat;
        core_stat.available = w_stat.available;
        core_stat.system = w_stat.system;
        core_stat.steal = w_stat.steal;
        core_stat.error = w_stat.error;
        core_stat.user = w_stat.user;
        core_stat.utilization = w_stat.utilization;

        std::transform(w_stat.targets.begin(), w_stat.targets.end(),
            std::back_inserter(core_stat.targets),
            [](const auto & i) -> model::generator_target_stats {
                return { .operations = i.operations };
            });

        stats.cores.push_back(core_stat);
    }

    if (stats.steal == 0ns)
        stats.steal = cpu_steal_time();

    auto gen_stat = model::generator_result{};
    gen_stat.id(m_result_id);
    gen_stat.generator_id(id());
    gen_stat.active(running());
    gen_stat.timestamp(timesync::chrono::realtime::now());
    gen_stat.stats(stats);
    return gen_stat;
}

void generator::clear_statistics() {
    for (auto & worker : m_workers)
        worker->clear_stat();

    m_result_id = core::to_string(core::uuid::random());
}

void generator::configure_workers(const model::generator_config& config) {
    m_workers.clear();

    auto cores_count = cpu_cores();
    if (static_cast<int32_t>(config.cores.size()) > cores_count)
        throw std::runtime_error(
            "Could not configure more cores than available ("
            + std::to_string(cores_count) + ").");

    for (size_t core = 0; core < config.cores.size(); ++core) {
        auto core_conf = config.cores.at(core);
        for (const auto & target : core_conf.targets)
            if (!is_supported(target.instruction_set))
                throw std::runtime_error("Instruction set "
                    + to_string(target.instruction_set)
                    + " is not supported");

        m_workers.push_back(std::make_unique<cpu_worker>(
            to_task_config(core_conf),
            "cpu" + std::to_string(m_serial_number)
            + "_" + std::to_string(core)));

        if (core_conf.utilization > 0.0)
            m_workers.back()->start(core);
    }
}

task_cpu_config generator::to_task_config(const model::generator_core_config& conf)
{
    if (conf.targets.empty() && conf.utilization > 0.0)
        throw std::runtime_error("Undefined configuration");

    task_cpu_config w_config;
    w_config.utilization = conf.utilization;

    std::transform(conf.targets.begin(), conf.targets.end(),
        std::back_inserter(w_config.targets),
        [](const auto & i) -> target_config {
            return {
                .set = i.instruction_set,
                .weight = i.weight,
                .data_type = i.data_type,
            };
        });

    return w_config;
}

bool generator::is_supported(cpu::instruction_set iset)
{
    switch (iset) {
    case cpu::instruction_set::SCALAR: return true;
    default: return false;
    }
}

} // namespace openperf::cpu::generator
