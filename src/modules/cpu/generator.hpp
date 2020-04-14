#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::cpu::generator {

using cpu_result_ptr = std::shared_ptr<model::cpu_generator_result>;

class cpu_generator : public model::cpu_generator
{
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