#include "common.hpp"

namespace openperf::cpu {

cpu::instruction_set to_instruction_set(const std::string& value)
{
    const static std::unordered_map<std::string, cpu::instruction_set>
        cpu_instruction_sets = {
            {"scalar", cpu::instruction_set::SCALAR},
    };

    if (cpu_instruction_sets.count(value))
        return cpu_instruction_sets.at(value);
    throw std::runtime_error(
        "Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const cpu::instruction_set& pattern)
{
    const static std::unordered_map<cpu::instruction_set, std::string>
        cpu_instruction_set_strings = {
            {cpu::instruction_set::SCALAR, "scalar"},
    };

    if (cpu_instruction_set_strings.count(pattern))
        return cpu_instruction_set_strings.at(pattern);
    return "unknown";
}

cpu::data_type to_data_type(const std::string& value)
{
    const static std::unordered_map<std::string, cpu::data_type>
        cpu_data_types = {
            {"int32", cpu::data_type::INT32},
            {"int64", cpu::data_type::INT64},
            {"float32", cpu::data_type::FLOAT32},
            {"float64", cpu::data_type::FLOAT64},
    };

    if (cpu_data_types.count(value))
        return cpu_data_types.at(value);
    throw std::runtime_error("Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const cpu::data_type& pattern)
{
    const static std::unordered_map<cpu::data_type, std::string>
        cpu_data_type_strings = {
            {cpu::data_type::INT32, "int32"},
            {cpu::data_type::INT64, "int64"},
            {cpu::data_type::FLOAT32, "float32"},
            {cpu::data_type::FLOAT64, "float64"},
    };

    if (cpu_data_type_strings.count(pattern))
        return cpu_data_type_strings.at(pattern);
    return "unknown";
}

} // namespace openperf::cpu
