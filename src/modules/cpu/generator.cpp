#include <cstring>
#include <stdexcept>
#include "generator.hpp"
#include "core/op_uuid.hpp"

namespace openperf::cpu::generator {

cpu_generator::cpu_generator(const model::cpu_generator& generator_model)
    : model::cpu_generator(generator_model)
{
    for (auto & core_conf : get_config().cores) {
        auto cc = generate_worker_config(core_conf);
        m_workers.push_back(std::make_unique<cpu_worker>(cc));
        m_workers.back()->start();
        m_workers.back()->config(cc);
    }
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

    m_workers.clear();

    for (auto & core_conf : p_conf.cores) {
        auto cc = generate_worker_config(core_conf);
        m_workers.push_back(std::make_unique<cpu_worker>(cc));
        m_workers.back()->start();
        m_workers.back()->config(cc);
    }

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
    gen_stat->set_stats({});
    return gen_stat;
}

void cpu_generator::clear_statistics() { }


task_config_t cpu_generator::generate_worker_config(const model::cpu_generator_core_config&)
{
    task_config_t w_config;
    return w_config;
}

} // namespace openperf::cpu::generator