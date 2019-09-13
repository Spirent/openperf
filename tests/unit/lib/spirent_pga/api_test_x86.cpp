#include "api_test.h"

namespace pga::test {

instruction_set_list instruction_sets()
{
    return { instruction_set::type::SCALAR,
             instruction_set::type::SSE2,
             instruction_set::type::SSE4,
             instruction_set::type::AVX,
             instruction_set::type::AVX2,
             instruction_set::type::AVX512SKX };
}

instruction_set_list vector_instruction_sets()
{
    return { instruction_set::type::SSE2,
             instruction_set::type::SSE4,
             instruction_set::type::AVX,
             instruction_set::type::AVX2,
             instruction_set::type::AVX512SKX };
}

}
