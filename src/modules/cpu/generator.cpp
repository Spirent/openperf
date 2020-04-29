#include <cstring>
#include <stdexcept>
#include "generator.hpp"
#include "core/op_uuid.hpp"
#include "api.hpp"
#include "lib/spirent_pga/instruction_set.h"
#include "cpu_info.hpp"

namespace openperf::cpu::generator {

const static std::unordered_map<model::cpu_instruction_set, instruction_set>
    cpu_instruction_sets = {
        {model::cpu_instruction_set::SCALAR, instruction_set::SCALAR},
};

const static std::unordered_map<uint32_t, target::data_size_t>
    cpu_data_sizes = {
        {32, target::data_size_t::BIT_32},
        {64, target::data_size_t::BIT_64},
};

const static std::unordered_map<model::cpu_operation, target::operation_t>
    cpu_operations = {
        {model::cpu_operation::INT, target::operation_t::INT},
        {model::cpu_operation::FLOAT, target::operation_t::FLOAT},
};

generator::generator(const model::generator& generator_model)
    : model::generator(generator_model),
    result_id(core::to_string(core::uuid::random()))
{
    configure_workers(config());
}

generator::~generator()
{
    for (auto & worker : m_workers)
        worker->stop();
}

model::generator_result generator::start() {
    set_running(true);
    return statistics();
}

void generator::stop() { set_running(false); }

void generator::set_config(const model::generator_config& p_conf)
{
    if (running())
        throw std::runtime_error("Cannot change config of running generator");

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
    auto gen_stat = model::generator_result{};
    gen_stat.id(result_id);
    gen_stat.generator_id(id());
    gen_stat.active(running());
    gen_stat.timestamp(model::time_point::clock::now());
    model::generator_stats stats;
    for (auto & worker : m_workers) {
        stats.cores.push_back({});
        for (auto & target : worker->stat().targets) {
            stats.cores.back().targets.push_back({target.cycles});
        }
    }
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

    if (static_cast<int32_t>(p_conf.cores.size()) > info::cores_count())
        throw std::runtime_error("Could not configure more cores than available (" + std::to_string(info::cores_count()) + ")?");

    for (auto & core_conf : p_conf.cores) {
        for (auto &target : core_conf.targets)
            if (!check_instruction_set_supported(target.instruction_set))
                throw std::runtime_error("Instruction set " + api::to_string(target.instruction_set) + " is not supported");
        auto cc = generate_worker_config(core_conf);
        m_workers.push_back(std::make_unique<cpu_worker>(cc));
        m_workers.back()->start(++core_id);
        m_workers.back()->config(cc);
    }
}

task_cpu_config generator::generate_worker_config(const model::generator_core_config& p_conf)
{
    task_cpu_config w_config;
    try {
        for (auto & target : p_conf.targets) {
            w_config.targets.push_back(target_config{
                .set = cpu_instruction_sets.at(target.instruction_set),
                .weight = target.weight,
                .data_size = cpu_data_sizes.at(target.data_size),
                .operation = cpu_operations.at(target.operation)
            });

        }
    } catch (const std::out_of_range& e) {
        throw std::runtime_error("Undefined configuration");
    }

    return w_config;
}

bool generator::check_instruction_set_supported(model::cpu_instruction_set p_iset)
{
    using namespace pga::instruction_set;
    switch (p_iset)
    {
    case model::cpu_instruction_set::SCALAR:
        return available(type::SCALAR);
        break;
    }

    return false;
}

} // namespace openperf::cpu::generator