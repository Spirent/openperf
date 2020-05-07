/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

/*
 * Self-contained version of DPDK's memcpy routines
 * Note: this is the AVX2 version
 */

#ifndef _OP_UTILS_DPDK_MEMCPY_AVX2_HPP_
#define _OP_UTILS_DPDK_MEMCPY_AVX2_HPP_

#include <stdint.h>
#include <x86intrin.h>

namespace dpdk::impl::avx2 {

inline constexpr int alignment_mask = 0x1f;

/**
 * AVX2 implementation below
 */

/**
 * Copy 16 bytes from one location to another,
 * locations should not overlap.
 */
static inline __attribute__((always_inline)) void rte_mov16(uint8_t* dst,
                                                            const uint8_t* src)
{
    __m128i xmm0;

    xmm0 = _mm_loadu_si128((const __m128i*)src);
    _mm_storeu_si128((__m128i*)dst, xmm0);
}

/**
 * Copy 32 bytes from one location to another,
 * locations should not overlap.
 */
static inline __attribute__((always_inline)) void rte_mov32(uint8_t* dst,
                                                            const uint8_t* src)
{
    __m256i ymm0;

    ymm0 = _mm256_loadu_si256((const __m256i*)src);
    _mm256_storeu_si256((__m256i*)dst, ymm0);
}

/**
 * Copy 64 bytes from one location to another,
 * locations should not overlap.
 */
static inline __attribute__((always_inline)) void rte_mov64(uint8_t* dst,
                                                            const uint8_t* src)
{
    rte_mov32((uint8_t*)dst + 0 * 32, (const uint8_t*)src + 0 * 32);
    rte_mov32((uint8_t*)dst + 1 * 32, (const uint8_t*)src + 1 * 32);
}

/**
 * Copy 128 bytes from one location to another,
 * locations should not overlap.
 */
static inline void rte_mov128(uint8_t* dst, const uint8_t* src)
{
    rte_mov32((uint8_t*)dst + 0 * 32, (const uint8_t*)src + 0 * 32);
    rte_mov32((uint8_t*)dst + 1 * 32, (const uint8_t*)src + 1 * 32);
    rte_mov32((uint8_t*)dst + 2 * 32, (const uint8_t*)src + 2 * 32);
    rte_mov32((uint8_t*)dst + 3 * 32, (const uint8_t*)src + 3 * 32);
}

/**
 * Copy 128-byte blocks from one location to another,
 * locations should not overlap.
 */
static inline void rte_mov128blocks(uint8_t* dst, const uint8_t* src, size_t n)
{
    __m256i ymm0, ymm1, ymm2, ymm3;

    while (n >= 128) {
        ymm0 =
            _mm256_loadu_si256((const __m256i*)((const uint8_t*)src + 0 * 32));
        n -= 128;
        ymm1 =
            _mm256_loadu_si256((const __m256i*)((const uint8_t*)src + 1 * 32));
        ymm2 =
            _mm256_loadu_si256((const __m256i*)((const uint8_t*)src + 2 * 32));
        ymm3 =
            _mm256_loadu_si256((const __m256i*)((const uint8_t*)src + 3 * 32));
        src = (const uint8_t*)src + 128;
        _mm256_storeu_si256((__m256i*)((uint8_t*)dst + 0 * 32), ymm0);
        _mm256_storeu_si256((__m256i*)((uint8_t*)dst + 1 * 32), ymm1);
        _mm256_storeu_si256((__m256i*)((uint8_t*)dst + 2 * 32), ymm2);
        _mm256_storeu_si256((__m256i*)((uint8_t*)dst + 3 * 32), ymm3);
        dst = (uint8_t*)dst + 128;
    }
}

inline void* rte_memcpy_generic(void* dst, const void* src, size_t n)
{
    uintptr_t dstu = (uintptr_t)dst;
    uintptr_t srcu = (uintptr_t)src;
    void* ret = dst;
    size_t dstofss;
    size_t bits;

    /**
     * Copy less than 16 bytes
     */
    if (n < 16) {
        if (n & 0x01) {
            *(uint8_t*)dstu = *(const uint8_t*)srcu;
            srcu = (uintptr_t)((const uint8_t*)srcu + 1);
            dstu = (uintptr_t)((uint8_t*)dstu + 1);
        }
        if (n & 0x02) {
            *(uint16_t*)dstu = *(const uint16_t*)srcu;
            srcu = (uintptr_t)((const uint16_t*)srcu + 1);
            dstu = (uintptr_t)((uint16_t*)dstu + 1);
        }
        if (n & 0x04) {
            *(uint32_t*)dstu = *(const uint32_t*)srcu;
            srcu = (uintptr_t)((const uint32_t*)srcu + 1);
            dstu = (uintptr_t)((uint32_t*)dstu + 1);
        }
        if (n & 0x08) { *(uint64_t*)dstu = *(const uint64_t*)srcu; }
        return ret;
    }

    /**
     * Fast way when copy size doesn't exceed 256 bytes
     */
    if (n <= 32) {
        rte_mov16((uint8_t*)dst, (const uint8_t*)src);
        rte_mov16((uint8_t*)dst - 16 + n, (const uint8_t*)src - 16 + n);
        return ret;
    }
    if (n <= 48) {
        rte_mov16((uint8_t*)dst, (const uint8_t*)src);
        rte_mov16((uint8_t*)dst + 16, (const uint8_t*)src + 16);
        rte_mov16((uint8_t*)dst - 16 + n, (const uint8_t*)src - 16 + n);
        return ret;
    }
    if (n <= 64) {
        rte_mov32((uint8_t*)dst, (const uint8_t*)src);
        rte_mov32((uint8_t*)dst - 32 + n, (const uint8_t*)src - 32 + n);
        return ret;
    }
    if (n <= 256) {
        if (n >= 128) {
            n -= 128;
            rte_mov128((uint8_t*)dst, (const uint8_t*)src);
            src = (const uint8_t*)src + 128;
            dst = (uint8_t*)dst + 128;
        }
    COPY_BLOCK_128_BACK31:
        if (n >= 64) {
            n -= 64;
            rte_mov64((uint8_t*)dst, (const uint8_t*)src);
            src = (const uint8_t*)src + 64;
            dst = (uint8_t*)dst + 64;
        }
        if (n > 32) {
            rte_mov32((uint8_t*)dst, (const uint8_t*)src);
            rte_mov32((uint8_t*)dst - 32 + n, (const uint8_t*)src - 32 + n);
            return ret;
        }
        if (n > 0) {
            rte_mov32((uint8_t*)dst - 32 + n, (const uint8_t*)src - 32 + n);
        }
        return ret;
    }

    /**
     * Make store aligned when copy size exceeds 256 bytes
     */
    dstofss = (uintptr_t)dst & 0x1F;
    if (dstofss > 0) {
        dstofss = 32 - dstofss;
        n -= dstofss;
        rte_mov32((uint8_t*)dst, (const uint8_t*)src);
        src = (const uint8_t*)src + dstofss;
        dst = (uint8_t*)dst + dstofss;
    }

    /**
     * Copy 128-byte blocks
     */
    rte_mov128blocks((uint8_t*)dst, (const uint8_t*)src, n);
    bits = n;
    n = n & 127;
    bits -= n;
    src = (const uint8_t*)src + bits;
    dst = (uint8_t*)dst + bits;

    /**
     * Copy whatever left
     */
    goto COPY_BLOCK_128_BACK31;
}

inline void* rte_memcpy_aligned(void* dst, const void* src, size_t n)
{
    void* ret = dst;

    /* Copy size <= 16 bytes */
    if (n < 16) {
        if (n & 0x01) {
            *(uint8_t*)dst = *(const uint8_t*)src;
            src = (const uint8_t*)src + 1;
            dst = (uint8_t*)dst + 1;
        }
        if (n & 0x02) {
            *(uint16_t*)dst = *(const uint16_t*)src;
            src = (const uint16_t*)src + 1;
            dst = (uint16_t*)dst + 1;
        }
        if (n & 0x04) {
            *(uint32_t*)dst = *(const uint32_t*)src;
            src = (const uint32_t*)src + 1;
            dst = (uint32_t*)dst + 1;
        }
        if (n & 0x08) *(uint64_t*)dst = *(const uint64_t*)src;

        return ret;
    }

    /* Copy 16 <= size <= 32 bytes */
    if (n <= 32) {
        rte_mov16((uint8_t*)dst, (const uint8_t*)src);
        rte_mov16((uint8_t*)dst - 16 + n, (const uint8_t*)src - 16 + n);

        return ret;
    }

    /* Copy 32 < size <= 64 bytes */
    if (n <= 64) {
        rte_mov32((uint8_t*)dst, (const uint8_t*)src);
        rte_mov32((uint8_t*)dst - 32 + n, (const uint8_t*)src - 32 + n);

        return ret;
    }

    /* Copy 64 bytes blocks */
    for (; n >= 64; n -= 64) {
        rte_mov64((uint8_t*)dst, (const uint8_t*)src);
        dst = (uint8_t*)dst + 64;
        src = (const uint8_t*)src + 64;
    }

    /* Copy whatever left */
    rte_mov64((uint8_t*)dst - 64 + n, (const uint8_t*)src - 64 + n);

    return ret;
}

} // namespace dpdk::impl::avx2

#endif /* _OP_UTILS_DPDK_MEMCPY_AVX2_HPP_ */
