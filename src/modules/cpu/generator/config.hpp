#ifndef _OP_CPU_GENERATOR_CONFIG_HPP_
#define _OP_CPU_GENERATOR_CONFIG_HPP_

#include <atomic>
#include <thread>
#include <vector>

#include "cpu/arg_parser.hpp"
#include "cpu/generator/worker_api.hpp"
#include "cpu/generator/results.hpp"

namespace openperf::cpu::generator {

template <typename InstructionSet, typename DataType>
struct target_operations_config_impl
{
    using instruction_set_type = InstructionSet;
    using data_type = DataType;

    int weight;
};

template <typename... Pairs>
std::variant<target_operations_config_impl<typename Pairs::first_type,
                                           typename Pairs::second_type>...>
    as_target_operations_config_variant(type_list<Pairs...>);

using target_op_config =
    decltype(as_target_operations_config_variant(load_types{}));

struct core_config
{
    double utilization;
    std::vector<target_op_config> targets;
};

struct config
{
    std::optional<core::cpuset> cores;
    std::vector<core_config> core_configs;
    bool is_system_config;
};

} // namespace openperf::cpu::generator

#endif // _OP_CPU_GENERATOR_CONFIG_HPP_
