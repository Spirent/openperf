#include "test_ispc_target.hpp"

namespace cpu::test {

using isa_type = openperf::cpu::instruction_set::type;

instruction_sets vector_instruction_sets()
{
    return {isa_type::SSE2,
            isa_type::SSE4,
            isa_type::AVX,
            isa_type::AVX2,
            isa_type::AVX512SKX};
}

} // namespace cpu::test
