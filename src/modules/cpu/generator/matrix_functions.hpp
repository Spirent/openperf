#ifndef _OP_CPU_GENERATOR_MATRIX_FUNCTIONS_HPP_
#define _OP_CPU_GENERATOR_MATRIX_FUNCTIONS_HPP_

#include <stdint.h>

#include "cpu/generator/function_wrapper.hpp"
#include "utils/singleton.hpp"

ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_double,
                           const double[],
                           const double[],
                           double[],
                           size_t);

ISPC_FUNCTION_WRAPPER_INIT(
    void, multiply_matrix_float, const float[], const float[], float[], size_t);

ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_int32,
                           const int32_t[],
                           const int32_t[],
                           int32_t[],
                           size_t);
ISPC_FUNCTION_WRAPPER_INIT(void,
                           multiply_matrix_int64,
                           const int64_t[],
                           const int64_t[],
                           int64_t[],
                           size_t);

namespace openperf::cpu::generator {

using multiply_matrix_double_fn = void (*)(const double[],
                                           const double[],
                                           double[],
                                           size_t);

using multiply_matrix_float_fn = void (*)(const float[],
                                          const float[],
                                          float[],
                                          size_t);

using multiply_matrix_int32_fn = void (*)(const int32_t[],
                                          const int32_t[],
                                          int32_t[],
                                          size_t);

using multiply_matrix_int64_fn = void (*)(const int64_t[],
                                          const int64_t[],
                                          int64_t[],
                                          size_t);

struct matrix_functions : utils::singleton<matrix_functions>
{
    function_wrapper<multiply_matrix_double_fn> multiply_matrix_double;
    function_wrapper<multiply_matrix_float_fn> multiply_matrix_float;
    function_wrapper<multiply_matrix_int32_fn> multiply_matrix_int32;
    function_wrapper<multiply_matrix_int64_fn> multiply_matrix_int64;
};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_MATRIX_FUNCTIONS_HPP_ */
