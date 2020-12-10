#include "api_test.h"

namespace pga::test {

instruction_set_list instruction_sets()
{
    return {instruction_set::type::SCALAR, instruction_set::type::AUTO};
}

instruction_set_list vector_instruction_sets()
{
    return {instruction_set::type::AUTO};
}

} // namespace pga::test
