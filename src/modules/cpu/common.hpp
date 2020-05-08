#ifndef _OP_CPU_COMMON_HPP_
#define _OP_CPU_COMMON_HPP_

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

} // namespace openperf::cpu::internal

#endif // _OP_CPU_COMMON_HPP_
