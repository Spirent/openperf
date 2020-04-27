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
using cpu_result_ptr = std::shared_ptr<model::cpu_generator_result>;

class cpu_generator : public model::cpu_generator
{
    cpu_worker_vec m_workers;
    void configure_workers(const model::cpu_generator_config& p_conf);
    bool check_instruction_set_supported(model::cpu_instruction_set);
    task_config_t generate_worker_config(const model::cpu_generator_core_config&);

public:
    ~cpu_generator();
    cpu_generator(const model::cpu_generator& generator_model);
    cpu_result_ptr start();
    void stop();

    void set_config(const model::cpu_generator_config& value);
    void set_running(bool value);

    cpu_result_ptr get_statistics() const;
    void clear_statistics();

};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_HPP_ */