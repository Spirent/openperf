#ifndef _ICP_SOCKET_DPDK_MEMCPY_H_
#define _ICP_SOCKET_DPDK_MEMCPY_H_

#include <cstdint>
#include <cstring>

#include "memcpy_avx512.h"
#include "memcpy_avx2.h"
#include "memcpy_sse.h"

/* We should _always_ have libc */
static constexpr bool libc_enabled = true;

/* Turn compiler generated instruction set macros into constexpr booleans */
#if defined (__AVX512F__)
static constexpr bool avx512_enabled = true;
#else
static constexpr bool avx512_enabled = false;
#endif

#if defined (__AVX2__) && ! defined (__AVX512F__)
static constexpr bool avx2_enabled = true;
#else
static constexpr bool avx2_enabled = false;
#endif

#if defined (__SSE4_2__) && ! defined (__AVX2__)
static constexpr bool sse_enabled = true;
#else
static constexpr bool sse_enabled = false;
#endif

namespace dpdk {

static inline void *
memcpy(void *dst, const void *src, size_t n)
{
    if constexpr (avx512_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst) | reinterpret_cast<uintptr_t>(src)) & impl::avx512::alignment_mask)) {
            return impl::avx512::rte_memcpy_aligned(dst, src, n);
        } else {
            return impl::avx512::rte_memcpy_generic(dst, src, n);
        }
    }
    if constexpr (avx2_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst) | reinterpret_cast<uintptr_t>(src)) & impl::avx2::alignment_mask)) {
            return impl::avx2::rte_memcpy_aligned(dst, src, n);
        } else {
            return impl::avx2::rte_memcpy_generic(dst, src, n);
        }
    }
    if constexpr (sse_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst) | reinterpret_cast<uintptr_t>(src)) & impl::sse::alignment_mask)) {
            return impl::sse::rte_memcpy_aligned(dst, src, n);
        } else {
            return impl::sse::rte_memcpy_generic(dst, src, n);
        }
    }
    if constexpr (libc_enabled) {
        return std::memcpy(dst, src, n);
    }
}

}

#endif /* _ICP_SOCKET_DPDK_MEMCPY_H_ */
