#ifndef _OP_CPU_ISPC_MATRIX_HPP_
#define _OP_CPU_ISPC_MATRIX_HPP_

#include <cinttypes>
#include "function_wrapper.hpp"

namespace openperf::cpu::internal {

ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_double,
                           const double[],
                           const double[],
                           double[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_float,
                           const float[],
                           const float[],
                           float[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_uint32,
                           const uint32_t[],
                           const uint32_t[],
                           uint32_t[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_uint64,
                           const uint64_t[],
                           const uint64_t[],
                           uint64_t[],
                           uint32_t);

} // namespace openperf::cpu::internal

#endif // _OP_CPU_ISPC_MATRIX_HPP_
