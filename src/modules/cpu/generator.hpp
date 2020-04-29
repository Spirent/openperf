#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "utils/worker/worker.hpp"
#include "temp_task.hpp"

namespace openperf::cpu::generator {

using namespace openperf::cpu::worker;

using cpu_worker = utils::worker::worker<cpu_task>;
using cpu_worker_ptr = std::unique_ptr<cpu_worker>;
using cpu_worker_vec = std::vector<cpu_worker_ptr>;

class generator : public model::generator
{
    cpu_worker_vec m_workers;
    void configure_workers(const model::generator_config& p_conf);
    bool check_instruction_set_supported(model::cpu_instruction_set);
    task_config_t generate_worker_config(const model::generator_core_config&);

public:
    ~generator() override;
    generator(const model::generator& generator_model);

    void start();
    void stop();

    void set_config(const model::generator_config& value);
    void set_running(bool value);

    model::generator_result statistics() const;
    void clear_statistics();
};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_HPP_ */