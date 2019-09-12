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

template <typename FunctionPtr>
FunctionPtr get_function(const function_wrapper<FunctionPtr>& wrapper,
                         instruction_set::type type)
{
    return (pga::get_function(wrapper.functions, type));
}

}

#endif /* _LIB_SPIRENT_PGA_API_TEST_H_ */
