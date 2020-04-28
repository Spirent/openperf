#include <cstring>
#include <stdexcept>
#include "generator.hpp"
#include "core/op_uuid.hpp"
#include "api.hpp"
#include "lib/spirent_pga/instruction_set.h"
#include "cpu_info.hpp"

namespace openperf::cpu::generator {

cpu_generator::cpu_generator(const model::cpu_generator& generator_model)
    : model::cpu_generator(generator_model)
{
    configure_workers(get_config());
}

cpu_generator::~cpu_generator()
{
    for (auto & worker : m_workers)
        worker->stop();
}

cpu_result_ptr cpu_generator::start() {
    set_running(true);
    return get_statistics();
}

void cpu_generator::stop() { set_running(false); }

void cpu_generator::set_config(const model::cpu_generator_config& p_conf)
{
    if (is_running())
        throw std::runtime_error("Cannot change config of running generator");

    for (auto & worker : m_workers) {
        worker->stop();
    }

    configure_workers(p_conf);

    model::cpu_generator::set_config(p_conf);
}

void cpu_generator::set_running(bool is_running)
{
    for (auto & worker : m_workers) {
        if (is_running)
            worker->resume();
        else
            worker->pause();
    }
    model::cpu_generator::set_running(is_running);
}

cpu_result_ptr cpu_generator::get_statistics() const
{
    auto gen_stat = std::make_shared<model::cpu_generator_result>();
    gen_stat->set_id(core::to_string(core::uuid::random()));
    gen_stat->set_generator_id(get_id());
    gen_stat->set_active(is_running());
    gen_stat->set_timestamp(model::time_point::clock::now());
    model::cpu_generator_stats stats;
    for (auto & worker : m_workers) {
        stats.cores.push_back({
            .available = worker->stat().cycles
        });
    }
    gen_stat->set_stats(stats);
    return gen_stat;
}

void cpu_generator::clear_statistics() {
    for (auto & worker : m_workers) {
        worker->clear_stat();
    }
}

void cpu_generator::configure_workers(const model::cpu_generator_config& p_conf) {
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

task_config_t cpu_generator::generate_worker_config(const model::cpu_generator_core_config&)
{
    task_config_t w_config;
    return w_config;
}

bool cpu_generator::check_instruction_set_supported(model::cpu_instruction_set p_iset)
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