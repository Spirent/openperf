#ifndef _ICP_PACKETIO_STACK_ARCH_CC_H_
#define _ICP_PACKETIO_STACK_ARCH_CC_H_

/*
 * This file translates LWIP macros and keywords into sensible values for
 * the host platform and is included from <lwip>/src/include/lwip/arch.h
 */

#include "core/icp_core.h"

/*
 * Macro shenanigans to remove the enclosing parentheses from existing
 * LWIP_PLATFORM_DIAG invocations.
 */
#define _Args(...) __VA_ARGS__
#define STRIP_PARENS(X) X
#define PASS_PARAMETERS(X) STRIP_PARENS( _Args X )

#define LWIP_PLATFORM_DIAG(x)                       \
    do {                                            \
        icp_log(ICP_LOG_DEBUG, PASS_PARAMETERS(x)); \
    } while (0)

#define LWIP_PLATFORM_ASSERT(x)                 \
    do {                                        \
        icp_exit(x);                            \
    } while (0)

#endif /* _ICP_PACKETIO_STACK_ARCH_CC_H_ */
