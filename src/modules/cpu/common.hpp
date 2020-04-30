#ifndef _OP_CPU_COMMON_HPP_
#define _OP_CPU_COMMON_HPP_

namespace openperf::cpu {

enum class instruction_set {
    SCALAR = 1
};

enum class operation {
    INT = 1, FLOAT
};

enum class data_size {
    BIT_32 = 32,
    BIT_64 = 64
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_COMMON_HPP_
