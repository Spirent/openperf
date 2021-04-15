#include "test_ispc_target.hpp"

namespace cpu::test {

using isa_type = openperf::cpu::instruction_set::type;

instruction_sets vector_instruction_sets() { return {isa_type::NEON}; }

} // namespace cpu::test
