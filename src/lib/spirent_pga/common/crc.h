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

template<class T, size_t... N>
constexpr T bswap_impl(T i, std::index_sequence<N...>) {
    return (((i >> N * CHAR_BIT & uint8_t(-1)) << (sizeof(T) - 1 - N) * CHAR_BIT) | ...);
}

template<class T, class U = std::make_unsigned_t<T>>
constexpr U bswap(T i) {
  return bswap_impl<U>(i, std::make_index_sequence<sizeof(T)>{});
}

/* Declare variable at address aligned to a boundary */
#define DECLARE_ALIGNED(_declaration, _boundary)         \
        _declaration __attribute__((aligned(_boundary)))

/* Likely & unliekly hints for the compiler */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static constexpr DECLARE_ALIGNED(uint8_t crc_xmm_shift_tab[48], 16) = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/**
 * @brief Shifts right 128 bit register by specified number of bytes
 *
 * @param reg 128 bit value
 * @param num number of bytes to shift right \a reg by (0-16)
 *
 * @return \a reg >> (\a num * 8)
 */
__m128i xmm_shift_right(__m128i reg, const unsigned int num)
{
        const __m128i *p = (const __m128i *)(crc_xmm_shift_tab + 16 + num);

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
__m128i xmm_shift_left(__m128i reg, const unsigned int num)
{
        const __m128i *p = (const __m128i *)(crc_xmm_shift_tab + 16 - num);

        return _mm_shuffle_epi8(reg, _mm_loadu_si128(p));
}

/**
 * PCLMULQDQ CRC computation context structure
 */
struct crc_pclmulqdq_ctx {
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
__m128i crc32_folding_round(const __m128i data_block,
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
__m128i crc32_reduce_128_to_64(__m128i data128, const __m128i k3_q)
{
        __m128i tmp;

        tmp = _mm_xor_si128(_mm_clmulepi64_si128(data128, k3_q, 0x01 /* k3 */),
                            data128);

        data128 = _mm_xor_si128(_mm_clmulepi64_si128(tmp, k3_q, 0x01 /* k3 */),
                                data128);

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
uint32_t
crc32_reduce_64_to_32(__m128i fold, const __m128i k3_q, const __m128i p_res)
{
        __m128i temp;

        temp = _mm_clmulepi64_si128(_mm_srli_si128(fold, 4),
                                    k3_q, 0x10 /* Q */);
        temp = _mm_srli_si128(_mm_xor_si128(temp, fold), 4);
        temp = _mm_clmulepi64_si128(temp, p_res, 0 /* P */);
        return _mm_extract_epi32(_mm_xor_si128(temp, fold), 0);
}

/**
 * @brief Calculates 32 bit CRC for given \a data block by applying folding and
 *        reduction methods.
 *
 * Algorithm operates on 32 bit CRCs so polynomials and initial values may
 * need to be promoted to 32 bits where required.
 *
 * @param crc initial CRC value (32 bit value)
 * @param data pointer to data block
 * @param data_len length of \a data block in bytes
 * @param params pointer to PCLMULQDQ CRC calculation context
 *
 * @return CRC for given \a data block (32 bits wide).
 */
uint32_t
crc32_calc_pclmulqdq(const uint8_t *data,
                     uint32_t data_len, uint32_t crc,
                     const struct crc_pclmulqdq_ctx *params)
{
        __m128i temp, fold, k, swap;
        uint32_t n;

        if (unlikely(data == NULL || data_len == 0 || params == NULL))
                return crc;

        /**
         * Add 4 bytes to data block size
         * This is to secure the following:
         *     CRC32 = M(X)^32 mod P(X)
         * M(X) - message to compute CRC on
         * P(X) - CRC polynomial
         */
        data_len += 4;

        /**
         * Load first 16 data bytes in \a fold and
         * set \a swap BE<->LE 16 byte conversion variable
         */
        fold = _mm_loadu_si128((__m128i *)data);
        swap = _mm_setr_epi32(0x0c0d0e0f, 0x08090a0b,
                              0x04050607, 0x00010203);

        /**
         * -------------------------------------------------
         * Folding all data into single 16 byte data block
         * Assumes: \a fold holds first 16 bytes of data
         */

        if (unlikely(data_len <= 16)) {
                /**
                 * Data block fits into 16 byte block
                 * - adjust data block
                 * - 4 least significant bytes need to be zero
                 */
                fold = _mm_shuffle_epi8(fold, swap);
                fold = _mm_slli_si128(xmm_shift_right(fold, 20 - data_len), 4);

                /**
                 * Apply CRC init value
                 */
                temp = _mm_insert_epi32(_mm_setzero_si128(), bswap(crc), 0);
                temp = xmm_shift_left(temp, data_len - 4);
                fold = _mm_xor_si128(fold, temp);
        } else {
                /**
                 * There are 2x16 data blocks or more
                 */
                __m128i next_data;

                /**
                 * n = number of bytes required to align \a data_len
                 *     to multiple of 16
                 */
                n = ((~data_len) + 1) & 15;

                /**
                 * Apply CRC initial value and
                 * get \a fold to BE format
                 */
                fold = _mm_xor_si128(fold,
                                     _mm_insert_epi32(_mm_setzero_si128(),
                                                      crc, 0));
                fold = _mm_shuffle_epi8(fold, swap);

                /**
                 * Load next 16 bytes of data and
                 * adjust \a fold & \a next_data as follows:
                 *
                 * CONCAT(fold,next_data) >> (n*8)
                 */
                next_data = _mm_loadu_si128((__m128i *)&data[16]);
                next_data = _mm_shuffle_epi8(next_data, swap);
                next_data = _mm_or_si128(xmm_shift_right(next_data, n),
                                         xmm_shift_left(fold, 16 - n));
                fold = xmm_shift_right(fold, n);

                if (unlikely(data_len <= 32))
                        /**
                         * In such unlikely case clear 4 least significant bytes
                         */
                        next_data =
                                _mm_slli_si128(_mm_srli_si128(next_data, 4), 4);

                /**
                 * Do the initial folding round on 2 first 16 byte chunks
                 */
                k = _mm_load_si128((__m128i *)(&params->k1));
                fold = crc32_folding_round(next_data, k, fold);

                if (likely(data_len > 32)) {
                        /**
                         * \a data_block needs to be at least 48 bytes long
                         * in order to get here
                         */
                        __m128i new_data;

                        /**
                         * Main folding loop
                         * - n is adjusted to point to next 16 data block
                         *   to read
                         *   (16+16) = 2x16; represents 2 first data blocks
                         *                   processed above
                         *   (- n) is the number of zero bytes padded to
                         *   the message in order to align it to 16 bytes
                         * - the last 16 bytes is processed separately
                         */
                        for (n = 16 + 16 - n; n < (data_len - 16); n += 16) {
                                new_data = _mm_loadu_si128((__m128i *)&data[n]);
                                new_data = _mm_shuffle_epi8(new_data, swap);
                                fold = crc32_folding_round(new_data, k, fold);
                        }

                        /**
                         * The last folding round works always on 12 bytes
                         * (12 bytes of data and 4 zero bytes)
                         * Read from offset -4 is to avoid one
                         * shift right operation.
                         */
                        new_data = _mm_loadu_si128((__m128i *)&data[n - 4]);
                        new_data = _mm_shuffle_epi8(new_data, swap);
                        new_data = _mm_slli_si128(new_data, 4);
                        fold = crc32_folding_round(new_data, k, fold);
                } /* if (data_len > 32) */
        }

        /**
         * -------------------------------------------------
         * Reduction 128 -> 32
         * Assumes: \a fold holds 128bit folded data
         */

        /**
         * REDUCTION 128 -> 64
         */
        k = _mm_load_si128((__m128i *)(&params->k3));
        fold = crc32_reduce_128_to_64(fold, k);

        /**
         * REDUCTION 64 -> 32
         */
        n = crc32_reduce_64_to_32(fold, k,
                                  _mm_load_si128((__m128i *)(&params->p)));

        return n;
}

#endif /* __CRC_H__ */
