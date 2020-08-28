#ifndef _OP_CPU_ISPC_MATRIX_HPP_
#define _OP_CPU_ISPC_MATRIX_HPP_

#include <cinttypes>
#include "function_wrapper.hpp"

namespace openperf::cpu::internal {

ISPC_FUNCTION_WRAPPER_INIT(
    void, multiplicate_matrix_double, double*, double*, double*, uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(
    void, multiplicate_matrix_float, float*, float*, float*, uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_uint32,
                           uint32_t*,
                           uint32_t*,
                           uint32_t*,
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiplicate_matrix_uint64,
                           uint64_t*,
                           uint64_t*,
                           uint64_t*,
                           uint32_t);

} // namespace openperf::cpu::internal

#endif // _OP_CPU_ISPC_MATRIX_HPP_
