#include "cpu/common.hpp"
#include "cpu/task_cpu.hpp"
#include "cpu/target_ispc.hpp"
#include "cpu/target_scalar.hpp"

namespace openperf::cpu::internal {

task_cpu::target_ptr task_cpu::make_target(instruction_set::type iset,
                                           data_type dtype)
{
    switch (iset) {
    case instruction_set::type::SCALAR:
        switch (dtype) {
        case data_type::INT32:
            return std::make_unique<target_scalar<uint32_t>>();
        case data_type::INT64:
            return std::make_unique<target_scalar<uint64_t>>();
        case data_type::FLOAT32:
            return std::make_unique<target_scalar<float>>();
        case data_type::FLOAT64:
            return std::make_unique<target_scalar<double>>();
        }
    case instruction_set::type::SSE2:
    case instruction_set::type::SSE4:
    case instruction_set::type::AVX:
    case instruction_set::type::AVX2:
    case instruction_set::type::AVX512SKX:
        switch (dtype) {
        case data_type::INT32:
            return std::make_unique<target_ispc<uint32_t>>(iset);
        case data_type::INT64:
            return std::make_unique<target_ispc<uint64_t>>(iset);
        case data_type::FLOAT32:
            return std::make_unique<target_ispc<float>>(iset);
        case data_type::FLOAT64:
            return std::make_unique<target_ispc<double>>(iset);
        }
    default:
        throw std::runtime_error("Unknown x86_64 instruction set "
                                 + std::string(to_string(iset)));
    }
}

} // namespace openperf::cpu::internal
