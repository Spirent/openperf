#ifndef _OP_CPU_COMMON_HPP_
#define _OP_CPU_COMMON_HPP_

#include <unordered_map>

namespace openperf::cpu {

enum class instruction_set {
    SCALAR = 1
};

enum class data_type {
    INT32 = 1,
    INT64,
    FLOAT32,
    FLOAT64
};

cpu::instruction_set to_instruction_set(const std::string& value);
std::string to_string(const cpu::instruction_set& pattern);

cpu::data_type to_data_type(const std::string& value);
std::string to_string(const cpu::data_type& pattern);

} // namespace openperf::cpu::internal

#endif // _OP_CPU_COMMON_HPP_
