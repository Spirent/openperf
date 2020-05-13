#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "utils/worker/worker.hpp"
#include "cpu/task_cpu.hpp"

namespace openperf::cpu::generator {

using namespace openperf::cpu::internal;

class generator : public model::generator
{
private:
    using cpu_worker = utils::worker::worker<task_cpu>;
    using cpu_worker_ptr = std::unique_ptr<cpu_worker>;
    using cpu_worker_vec = std::vector<cpu_worker_ptr>;

private:
    cpu_worker_vec m_workers;
    std::string result_id;

    void configure_workers(const model::generator_config& p_conf);
    bool check_instruction_set_supported(cpu::instruction_set);
    task_cpu_config generate_worker_config(const model::generator_core_config&);

public:
    generator(const model::generator& generator_model);
    ~generator() override;

    void start();
    void stop();

    void set_config(const model::generator_config& value);
    void set_running(bool value);

    model::generator_result statistics() const;
    void clear_statistics();
};

} // namespace openperf::cpu::generator

#endif // _OP_CPU_GENERATOR_HPP_
