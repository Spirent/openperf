#include "test_api.hpp"

namespace regurgitate::test {

instruction_sets get_instruction_sets()
{
    return {instruction_set::type::SSE2,
            instruction_set::type::SSE4,
            instruction_set::type::AVX,
            instruction_set::type::AVX2,
            instruction_set::type::AVX512SKX};
}

} // namespace regurgitate::test
