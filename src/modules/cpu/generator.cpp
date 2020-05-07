#include <cstring>
#include <stdexcept>
#include "generator.hpp"
#include "core/op_uuid.hpp"
#include "api.hpp"
#include "lib/spirent_pga/instruction_set.h"
#include "cpu_info.hpp"

namespace openperf::cpu::generator {

const static std::unordered_map<uint32_t, cpu::data_size>
    cpu_data_sizes = {
        {32, cpu::data_size::BIT_32},
        {64, cpu::data_size::BIT_64},
};

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

    for (auto & worker : m_workers) {
        worker->stop();
    }

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

        model::generator_core_stats core_stat;
        core_stat.available = w_stat.available;
        core_stat.system = w_stat.system;
        core_stat.steal = w_stat.steal;
        core_stat.error = w_stat.error;
        core_stat.user = w_stat.user;
        core_stat.utilization = w_stat.utilization;

        for (auto & target : w_stat.targets) {
            core_stat.targets.push_back({target.cycles});
        }

        stats.cores.push_back(core_stat);
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
    for (auto & worker : m_workers) {
        worker->clear_stat();
    }
    result_id = core::to_string(core::uuid::random());
}

void generator::configure_workers(const model::generator_config& p_conf) {
    m_workers.clear();
    int core_id = -1;

    std::cout << "Configure " << p_conf.cores.size() << std::endl;
    if (static_cast<int32_t>(p_conf.cores.size()) > info::cores_count())
        throw std::runtime_error(
            "Could not configure more cores than available ("
            + std::to_string(info::cores_count()) + ")?");

    for (auto & core_conf : p_conf.cores) {
        for (auto &target : core_conf.targets)
            if (!check_instruction_set_supported(target.instruction_set))
                throw std::runtime_error("Instruction set "
                    + api::to_string(target.instruction_set)
                    + " is not supported");

        std::cout << "Apply core config " << core_id
            << " with utilization " << core_conf.utilization
            << std::endl;
        auto cc = generate_worker_config(core_conf);
        m_workers.push_back(std::make_unique<cpu_worker>(cc));
        m_workers.back()->start(++core_id);
        //m_workers.back()->config(cc);
    }
}

task_cpu_config generator::generate_worker_config(const model::generator_core_config& conf)
{
    try {
        task_cpu_config w_config;
        w_config.utilization = conf.utilization;

        for (auto & target : conf.targets) {
            w_config.targets.push_back(target_config{
                .set = target.instruction_set,
                .weight = target.weight,
                .data_size = cpu_data_sizes.at(target.data_size),
                .operation = target.operation
            });
        }

        return w_config;
    } catch (const std::out_of_range& e) {
        throw std::runtime_error("Undefined configuration");
    }
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
