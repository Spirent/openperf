#ifndef _OP_CPU_ISPC_MATRIX_HPP_
#define _OP_CPU_ISPC_MATRIX_HPP_

#include <cinttypes>
#include "cpu/function_wrapper.hpp"

ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_double,
                           const double[],
                           const double[],
                           double[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_float,
                           const float[],
                           const float[],
                           float[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_uint32,
                           const uint32_t[],
                           const uint32_t[],
                           uint32_t[],
                           uint32_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_uint64,
                           const uint64_t[],
                           const uint64_t[],
                           uint64_t[],
                           uint32_t);

#endif // _OP_CPU_ISPC_MATRIX_HPP_
