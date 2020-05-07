#ifndef _OP_UTILS_MEMCPY_HPP_
#define _OP_UTILS_MEMCPY_HPP_

#include <cstdint>
#include <cstring>

#include "utils/dpdk/memcpy_avx512.hpp"
#include "utils/dpdk/memcpy_avx2.hpp"
#include "utils/dpdk/memcpy_sse.hpp"

/* Turn compiler generated instruction set macros into constexpr booleans */
#if defined(__AVX512F__)
static constexpr bool avx512_enabled = true;
#else
static constexpr bool avx512_enabled = false;
#endif

#if defined(__AVX2__) && !defined(__AVX512F__)
static constexpr bool avx2_enabled = true;
#else
static constexpr bool avx2_enabled = false;
#endif

#if defined(__SSSE3__) && !defined(__AVX2__)
static constexpr bool sse_enabled = true;
#else
static constexpr bool sse_enabled = false;
#endif

static_assert(avx512_enabled || avx2_enabled || sse_enabled);

namespace openperf::utils {

constexpr int memcpy_alignment_mask()
{
    if constexpr (avx512_enabled) {
        return (dpdk::impl::avx512::alignment_mask);
    }
    if constexpr (avx2_enabled) { return (dpdk::impl::avx2::alignment_mask); }
    if constexpr (sse_enabled) { return (dpdk::impl::sse::alignment_mask); }
}

constexpr int memcpy_alignment() { return (memcpy_alignment_mask() + 1); }

inline void* memcpy(void* dst, const void* src, size_t n)
{
    if constexpr (avx512_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst)
               | reinterpret_cast<uintptr_t>(src))
              & dpdk::impl::avx512::alignment_mask)) {
            return dpdk::impl::avx512::rte_memcpy_aligned(dst, src, n);
        } else {
            return dpdk::impl::avx512::rte_memcpy_generic(dst, src, n);
        }
    }
    if constexpr (avx2_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst)
               | reinterpret_cast<uintptr_t>(src))
              & dpdk::impl::avx2::alignment_mask)) {
            return dpdk::impl::avx2::rte_memcpy_aligned(dst, src, n);
        } else {
            return dpdk::impl::avx2::rte_memcpy_generic(dst, src, n);
        }
    }
    if constexpr (sse_enabled) {
        if (!((reinterpret_cast<uintptr_t>(dst)
               | reinterpret_cast<uintptr_t>(src))
              & dpdk::impl::sse::alignment_mask)) {
            return dpdk::impl::sse::rte_memcpy_aligned(dst, src, n);
        } else {
            return dpdk::impl::sse::rte_memcpy_generic(dst, src, n);
        }
    }
}

} // namespace openperf::utils

#endif /* _OP_UTILS_MEMCPY_HPP_ */
