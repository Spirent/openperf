#ifndef _MODULES_CPU_TEST_ISPC_TARGET_HPP_
#define _MODULES_CPU_TEST_ISPC_TARGET_HPP_

#include <vector>

#include "cpu/instruction_set.hpp"

namespace cpu::test {

using instruction_sets = std::vector<openperf::cpu::instruction_set::type>;

instruction_sets vector_instruction_sets();

} // namespace cpu::test

#endif /* _MODULES_CPU_TEST_ISPC_TARGET_HPP_ */
