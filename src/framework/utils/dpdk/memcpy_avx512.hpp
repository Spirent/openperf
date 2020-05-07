/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

/*
 * Self-contained version of DPDK's memcpy routines
 * Note: this is the AVX2 version
 */

#ifndef _OP_UTILS_DPDK_MEMCPY_AVX512_HPP_
#define _OP_UTILS_DPDK_MEMCPY_AVX512_HPP_

#include <stdint.h>
#include <x86intrin.h>

namespace dpdk::impl::avx512 {

inline constexpr int alignment_mask = 0x3f;

/**
 * AVX512 implementation below
 */

/**
 * Copy 16 bytes from one location to another,
 * locations should not overlap.
 */
static __attribute__((always_inline)) void rte_mov16(uint8_t* dst,
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
static __attribute__((always_inline)) void rte_mov32(uint8_t* dst,
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
static __attribute__((always_inline)) void rte_mov64(uint8_t* dst,
                                                     const uint8_t* src)
{
    __m512i zmm0;

    zmm0 = _mm512_loadu_si512((const void*)src);
    _mm512_storeu_si512((void*)dst, zmm0);
}

/**
 * Copy 128 bytes from one location to another,
 * locations should not overlap.
 */
static __attribute__((always_inline)) void rte_mov128(uint8_t* dst,
                                                      const uint8_t* src)
{
    rte_mov64(dst + 0 * 64, src + 0 * 64);
    rte_mov64(dst + 1 * 64, src + 1 * 64);
}

/**
 * Copy 256 bytes from one location to another,
 * locations should not overlap.
 */
static __attribute__((always_inline)) void rte_mov256(uint8_t* dst,
                                                      const uint8_t* src)
{
    rte_mov64(dst + 0 * 64, src + 0 * 64);
    rte_mov64(dst + 1 * 64, src + 1 * 64);
    rte_mov64(dst + 2 * 64, src + 2 * 64);
    rte_mov64(dst + 3 * 64, src + 3 * 64);
}

/**
 * Copy 128-byte blocks from one location to another,
 * locations should not overlap.
 */
static __attribute__((always_inline)) void
rte_mov128blocks(uint8_t* dst, const uint8_t* src, size_t n)
{
    __m512i zmm0, zmm1;

    while (n >= 128) {
        zmm0 = _mm512_loadu_si512((const void*)(src + 0 * 64));
        n -= 128;
        zmm1 = _mm512_loadu_si512((const void*)(src + 1 * 64));
        src = src + 128;
        _mm512_storeu_si512((void*)(dst + 0 * 64), zmm0);
        _mm512_storeu_si512((void*)(dst + 1 * 64), zmm1);
        dst = dst + 128;
    }
}

/**
 * Copy 512-byte blocks from one location to another,
 * locations should not overlap.
 */
static inline void rte_mov512blocks(uint8_t* dst, const uint8_t* src, size_t n)
{
    __m512i zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7;

    while (n >= 512) {
        zmm0 = _mm512_loadu_si512((const void*)(src + 0 * 64));
        n -= 512;
        zmm1 = _mm512_loadu_si512((const void*)(src + 1 * 64));
        zmm2 = _mm512_loadu_si512((const void*)(src + 2 * 64));
        zmm3 = _mm512_loadu_si512((const void*)(src + 3 * 64));
        zmm4 = _mm512_loadu_si512((const void*)(src + 4 * 64));
        zmm5 = _mm512_loadu_si512((const void*)(src + 5 * 64));
        zmm6 = _mm512_loadu_si512((const void*)(src + 6 * 64));
        zmm7 = _mm512_loadu_si512((const void*)(src + 7 * 64));
        src = src + 512;
        _mm512_storeu_si512((void*)(dst + 0 * 64), zmm0);
        _mm512_storeu_si512((void*)(dst + 1 * 64), zmm1);
        _mm512_storeu_si512((void*)(dst + 2 * 64), zmm2);
        _mm512_storeu_si512((void*)(dst + 3 * 64), zmm3);
        _mm512_storeu_si512((void*)(dst + 4 * 64), zmm4);
        _mm512_storeu_si512((void*)(dst + 5 * 64), zmm5);
        _mm512_storeu_si512((void*)(dst + 6 * 64), zmm6);
        _mm512_storeu_si512((void*)(dst + 7 * 64), zmm7);
        dst = dst + 512;
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
        if (n & 0x08) *(uint64_t*)dstu = *(const uint64_t*)srcu;
        return ret;
    }

    /**
     * Fast way when copy size doesn't exceed 512 bytes
     */
    if (n <= 32) {
        rte_mov16((uint8_t*)dst, (const uint8_t*)src);
        rte_mov16((uint8_t*)dst - 16 + n, (const uint8_t*)src - 16 + n);
        return ret;
    }
    if (n <= 64) {
        rte_mov32((uint8_t*)dst, (const uint8_t*)src);
        rte_mov32((uint8_t*)dst - 32 + n, (const uint8_t*)src - 32 + n);
        return ret;
    }
    if (n <= 512) {
        if (n >= 256) {
            n -= 256;
            rte_mov256((uint8_t*)dst, (const uint8_t*)src);
            src = (const uint8_t*)src + 256;
            dst = (uint8_t*)dst + 256;
        }
        if (n >= 128) {
            n -= 128;
            rte_mov128((uint8_t*)dst, (const uint8_t*)src);
            src = (const uint8_t*)src + 128;
            dst = (uint8_t*)dst + 128;
        }
    COPY_BLOCK_128_BACK63:
        if (n > 64) {
            rte_mov64((uint8_t*)dst, (const uint8_t*)src);
            rte_mov64((uint8_t*)dst - 64 + n, (const uint8_t*)src - 64 + n);
            return ret;
        }
        if (n > 0)
            rte_mov64((uint8_t*)dst - 64 + n, (const uint8_t*)src - 64 + n);
        return ret;
    }

    /**
     * Make store aligned when copy size exceeds 512 bytes
     */
    dstofss = ((uintptr_t)dst & 0x3F);
    if (dstofss > 0) {
        dstofss = 64 - dstofss;
        n -= dstofss;
        rte_mov64((uint8_t*)dst, (const uint8_t*)src);
        src = (const uint8_t*)src + dstofss;
        dst = (uint8_t*)dst + dstofss;
    }

    /**
     * Copy 512-byte blocks.
     * Use copy block function for better instruction order control,
     * which is important when load is unaligned.
     */
    rte_mov512blocks((uint8_t*)dst, (const uint8_t*)src, n);
    bits = n;
    n = n & 511;
    bits -= n;
    src = (const uint8_t*)src + bits;
    dst = (uint8_t*)dst + bits;

    /**
     * Copy 128-byte blocks.
     * Use copy block function for better instruction order control,
     * which is important when load is unaligned.
     */
    if (n >= 128) {
        rte_mov128blocks((uint8_t*)dst, (const uint8_t*)src, n);
        bits = n;
        n = n & 127;
        bits -= n;
        src = (const uint8_t*)src + bits;
        dst = (uint8_t*)dst + bits;
    }

    /**
     * Copy whatever left
     */
    goto COPY_BLOCK_128_BACK63;
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

} // namespace dpdk::impl::avx512

#endif /* _OP_UTILS_DPDK_MEMCPY_AVX512_HPP_ */
