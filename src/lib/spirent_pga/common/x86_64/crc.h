/*******************************************************************************
 Copyright (c) 2009-2018, Intel Corporation

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
 * Header file for module with CRC computation methods
 *
 * PCLMULQDQ implementation is based on work by:
 *               Erdinc Ozturk
 *               Vinodh Gopal
 *               James Guilford
 *
 * "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
 * URL: http://download.intel.com/design/intarch/papers/323102.pdf
 */

#ifndef __CRC_H__
#define __CRC_H__

#include <x86intrin.h>
#include <cstddef>
#include <climits>
#include <cstdint>
#include <utility>

/* Declare variable at address aligned to a boundary */
#define DECLARE_ALIGNED(_declaration, _boundary)                               \
    _declaration __attribute__((aligned(_boundary)))

/* Likely & unliekly hints for the compiler */
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static constexpr DECLARE_ALIGNED(uint8_t crc_xmm_shift_tab[48], 16) = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/**
 * PCLMULQDQ CRC computation context structure
 */
struct crc_pclmulqdq_ctx
{
    /**
     * K1 = reminder X^128 / P(X) : 0-63
     * K2 = reminder X^192 / P(X) : 64-127
     */
    uint64_t k1;
    uint64_t k2;

    /**
     * K3 = reminder X^64 / P(X) : 0-63
     * q  = quotient X^64 / P(X) : 64-127
     */
    uint64_t k3;
    uint64_t q;

    /**
     * p   = polynomial / P(X) : 0-63
     * res = reserved : 64-127
     */
    uint64_t p;
    uint64_t res;
};

/**
 * Functions and prototypes
 */

/**
 * @brief Shifts right 128 bit register by specified number of bytes
 *
 * @param reg 128 bit value
 * @param num number of bytes to shift right \a reg by (0-16)
 *
 * @return \a reg >> (\a num * 8)
 */
inline __m128i xmm_shift_right(__m128i reg, const unsigned int num)
{
    const __m128i* p = (const __m128i*)(crc_xmm_shift_tab + 16 + num);

    return _mm_shuffle_epi8(reg, _mm_loadu_si128(p));
}

/**
 * @brief Shifts left 128 bit register by specified number of bytes
 *
 * @param reg 128 bit value
 * @param num number of bytes to shift left \a reg by (0-16)
 *
 * @return \a reg << (\a num * 8)
 */
inline __m128i xmm_shift_left(__m128i reg, const unsigned int num)
{
    const __m128i* p = (const __m128i*)(crc_xmm_shift_tab + 16 - num);

    return _mm_shuffle_epi8(reg, _mm_loadu_si128(p));
}

/**
 * @brief Performs one folding round
 *
 * Logically function operates as follows:
 *     DATA = READ_NEXT_16BYTES();
 *     F1 = LSB8(FOLD)
 *     F2 = MSB8(FOLD)
 *     T1 = CLMUL( F1, K1 )
 *     T2 = CLMUL( F2, K2 )
 *     FOLD = XOR( T1, T2, DATA )
 *
 * @param data_block 16 byte data block
 * @param k1_k2 k1 and k2 constanst enclosed in XMM register
 * @param fold running 16 byte folded data
 *
 * @return New 16 byte folded data
 */
inline __m128i crc32_folding_round(const __m128i data_block,
                                   const __m128i k1_k2,
                                   const __m128i fold)
{
    __m128i tmp = _mm_clmulepi64_si128(fold, k1_k2, 0x11);

    return _mm_xor_si128(_mm_clmulepi64_si128(fold, k1_k2, 0x00),
                         _mm_xor_si128(data_block, tmp));
}

/**
 * @brief Performs Barret's reduction from 128 bits to 64 bits
 *
 * @param data128 128 bits data to be reduced
 * @param k3_q k3 and Q constants enclosed in XMM register
 *
 * @return data reduced to 64 bits
 */
inline __m128i crc32_reduce_128_to_64(__m128i data128, const __m128i k3_q)
{
    __m128i tmp;

    tmp = _mm_xor_si128(_mm_clmulepi64_si128(data128, k3_q, 0x01 /* k3 */),
                        data128);

    data128 =
        _mm_xor_si128(_mm_clmulepi64_si128(tmp, k3_q, 0x01 /* k3 */), data128);

    return _mm_srli_si128(_mm_slli_si128(data128, 8), 8);
}

/**
 * @brief Performs Barret's reduction from 64 bits to 32 bits
 *
 * @param data64 64 bits data to be reduced
 * @param k3_q k3 and Q constants enclosed in XMM register
 * @param p_res P constant enclosed in XMM register
 *
 * @return data reduced to 32 bits
 */
inline uint32_t
crc32_reduce_64_to_32(__m128i fold, const __m128i k3_q, const __m128i p_res)
{
    __m128i temp;

    temp = _mm_clmulepi64_si128(_mm_srli_si128(fold, 4), k3_q, 0x10 /* Q */);
    temp = _mm_srli_si128(_mm_xor_si128(temp, fold), 4);
    temp = _mm_clmulepi64_si128(temp, p_res, 0 /* P */);
    return _mm_extract_epi32(_mm_xor_si128(temp, fold), 0);
}

/**
 * @brief Calculate CRC16 for a 16 byte chunk of data, e.g. the signature
 *
 * Algorithm operates on 32 bit CRCs so polynomials and initial values may
 * need to be promoted to 32 bits where required.
 *
 * @param data pointer to data block
 *
 * @return CRC for given block
 */
inline uint32_t calculate_signature_crc16(const uint8_t data[])
{
    /* Spirent CRC16 constants */
    static constexpr DECLARE_ALIGNED(
        struct crc_pclmulqdq_ctx spirent_crc16_clmul, 16) = {
        0x45630000, /**< remainder X^128 / P(X) */
        0xd5f60000, /**< remainder X^192 / P(X) */
        0xaa510000, /**< remainder  X^64 / P(X) */
        0x11303471, /**< quotient   X^64 / P(X) */
        0x10210000, /**< polynomial */
        0ULL        /**< reserved */
    };

    /**
     * Load first 16 data bytes in \a fold and
     * set \a swap BE<->LE 16 byte conversion variable
     */
    __m128i fold = _mm_loadu_si128((__m128i*)data);
    __m128i swap =
        _mm_setr_epi32(0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203);

    /**
     * n = number of bytes required to align \a data_len
     *     to multiple of 16. We need 4 extra bytes of 0's
     *     to properly calculate the CRC, so this is based
     *     on a length of 20, not 16.
     */
    uint32_t n = 12;

    /**
     * Apply CRC initial value and
     * get \a fold to BE format
     */
    fold =
        _mm_xor_si128(fold, _mm_insert_epi32(_mm_setzero_si128(), 0xffff, 0));
    fold = _mm_shuffle_epi8(fold, swap);

    /**
     * We don't have any more data to load, so just fold in 0's
     */
    __m128i next_data = {0, 0};
    next_data = _mm_or_si128(xmm_shift_right(next_data, n),
                             xmm_shift_left(fold, 16 - n));
    fold = xmm_shift_right(fold, n);

    /**
     * Clear the 4 least significant bytes
     */
    next_data = _mm_slli_si128(_mm_srli_si128(next_data, 4), 4);

    /**
     * Fold our 2 x 16 byte chunks
     */
    __m128i k = _mm_load_si128((__m128i*)(&spirent_crc16_clmul.k1));
    fold = crc32_folding_round(next_data, k, fold);

    /**
     * -------------------------------------------------
     * Reduction 128 -> 32
     * Assumes: \a fold holds 128bit folded data
     */

    /**
     * REDUCTION 128 -> 64
     */
    k = _mm_load_si128((__m128i*)(&spirent_crc16_clmul.k3));
    fold = crc32_reduce_128_to_64(fold, k);

    /**
     * REDUCTION 64 -> 32
     */
    n = crc32_reduce_64_to_32(
        fold, k, _mm_load_si128((__m128i*)(&spirent_crc16_clmul.p)));

    return (~n >> 16);
}

#endif /* __CRC_H__ */
