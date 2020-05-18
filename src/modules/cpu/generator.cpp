#include "generator.hpp"

#include "cpu/cpu.hpp"
#include "core/op_uuid.hpp"
#include "lib/spirent_pga/instruction_set.h"

namespace openperf::cpu::generator {

generator::generator(const model::generator& generator_model)
    : model::generator(generator_model)
    , result_id(core::to_string(core::uuid::random()))
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
    for (auto & worker : m_workers) {
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

        for (auto & target : w_stat.targets)
            core_stat.targets.push_back({target.cycles});

        stats.cores.push_back(core_stat);
    }

    if (stats.steal == 0ns) {
        stats.steal = cpu_steal_time();
    }

    auto gen_stat = model::generator_result{};
    gen_stat.id(result_id);
    gen_stat.generator_id(id());
    gen_stat.active(running());
    gen_stat.timestamp(model::time_point::clock::now());
    gen_stat.stats(stats);
    return gen_stat;
}

void generator::clear_statistics() {
    for (auto & worker : m_workers)
        worker->clear_stat();

    result_id = core::to_string(core::uuid::random());
}

void generator::configure_workers(const model::generator_config& p_conf) {
    m_workers.clear();
    int core_id = -1;

    if (static_cast<int32_t>(p_conf.cores.size()) > cpu_cores_count())
        throw std::runtime_error(
            "Could not configure more cores than available ("
            + std::to_string(cpu_cores_count()) + ")?");

    for (auto & core_conf : p_conf.cores) {
        for (auto &target : core_conf.targets)
            if (!check_instruction_set_supported(target.instruction_set))
                throw std::runtime_error("Instruction set "
                    + to_string(target.instruction_set)
                    + " is not supported");

        auto cc = generate_worker_config(core_conf);
        m_workers.push_back(std::make_unique<cpu_worker>(cc));
        m_workers.back()->start(++core_id);
    }
}

task_cpu_config generator::generate_worker_config(const model::generator_core_config& conf)
{
    if (conf.targets.empty())
        throw std::runtime_error("Undefined configuration");

    task_cpu_config w_config;
    w_config.utilization = conf.utilization;

    for (auto & target : conf.targets) {
        w_config.targets.push_back(target_config{
            .set = target.instruction_set,
            .weight = target.weight,
            .data_type = target.data_type,
        });
    }

    return w_config;
}

bool generator::check_instruction_set_supported(cpu::instruction_set p_iset)
{
    using namespace pga::instruction_set;
    switch (p_iset)
    {
    case cpu::instruction_set::SCALAR:
        return available(type::SCALAR);
        break;
    }

    return false;
}

} // namespace openperf::cpu::generator
