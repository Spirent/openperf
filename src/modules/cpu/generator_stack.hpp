#ifndef _OP_CPU_GENERATOR_STACK_HPP_
#define _OP_CPU_GENERATOR_STACK_HPP_

#include <unordered_map>
#include "generator.hpp"
#include "tl/expected.hpp"

namespace openperf::cpu::generator {

using cpu_generator_ptr = std::shared_ptr<cpu_generator>;
using cpu_generator_map = std::unordered_map<std::string, cpu_generator_ptr>;

class generator_stack
{
private:
    cpu_generator_map m_generators;

public:
    generator_stack(){};

    std::vector<cpu_generator_ptr> cpu_generators_list() const;
    tl::expected<cpu_generator_ptr, std::string> create_cpu_generator(const model::cpu_generator& cpu_generator_model);
    cpu_generator_ptr get_cpu_generator(std::string id) const;
    void delete_cpu_generator(std::string id);
};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_STACK_HPP_ */