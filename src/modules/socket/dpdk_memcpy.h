/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

/*
 * Self-contained version of DPDK's memcpy routines
 * Note: this is the AVX2 version
 */

#ifndef _ICP_SOCKET_DPDK_MEMCPY_H_
#define _ICP_SOCKET_DPDK_MEMCPY_H_

#include "socket/memcpy/impl.h"

namespace dpdk {

static inline void * memcpy(void *dst, const void *src, size_t n)
{
    return impl::memcpy(dst, src, n);
}

}

#endif /* _ICP_SOCKET_DPDK_MEMCPY_H_ */
