#ifndef _LIB_SPIRENT_PGA_API_TEST_H_
#define _LIB_SPIRENT_PGA_API_TEST_H_

/**
 * @file
 *
 * This is a private API header for testing purposes only and not intended
 * for normal client use.
 */

#include "spirent_pga/functions.h"
#include "spirent_pga/instruction_set.h"

namespace pga::test {

template <typename FunctionPtr, typename Tag = void>
FunctionPtr get_function(const function_wrapper<FunctionPtr, Tag>& wrapper,
                         instruction_set::type type)
{
    return (pga::get_function(wrapper.functions, type));
}

using instruction_set_list = std::vector<instruction_set::type>;

instruction_set_list instruction_sets();
instruction_set_list vector_instruction_sets();
}

#endif /* _LIB_SPIRENT_PGA_API_TEST_H_ */
