#ifndef _OP_PACKETIO_STACK_DPDK_ARCH_CC_H_
#define _OP_PACKETIO_STACK_DPDK_ARCH_CC_H_

/*
 * This file translates LWIP macros and keywords into sensible values for
 * the host platform and is included from <lwip>/src/include/lwip/arch.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "core/op_log.h"

/*
 * Macro shenanigans to remove the enclosing parentheses from existing
 * LWIP_PLATFORM_DIAG invocations.
 */
#define _Args(...) __VA_ARGS__
#define STRIP_PARENS(X) X
#define PASS_PARAMETERS(X) STRIP_PARENS(_Args X)

#define LWIP_PLATFORM_DIAG(x)                                                  \
    do {                                                                       \
        op_log(OP_LOG_DEBUG, "lwip", PASS_PARAMETERS(x));                      \
    } while (0)

#define LWIP_PLATFORM_ASSERT(x)                                                \
    do {                                                                       \
        fprintf(stderr,                                                        \
                "Assertion \"%s\" failed at line %d in %s\n",                  \
                x,                                                             \
                __LINE__,                                                      \
                __FILE__);                                                     \
        fflush(stderr);                                                        \
        abort();                                                               \
    } while (0)

#ifndef __cplusplus
/*
 * Don't include this DPDK header for C++ code, as it uses the "register"
 * keyword, which is reserved in C++17.
 */
#include "rte_memcpy.h"
#endif
#define PACKETIO_MEMCPY(dst, src, len) rte_memcpy(dst, src, len)

#endif /* _OP_PACKETIO_STACK_DPDK_ARCH_CC_H_ */
