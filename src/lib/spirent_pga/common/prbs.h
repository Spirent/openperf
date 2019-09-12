#ifndef _LIB_SPIRENT_PGA_COMMON_PRBS_H_
#define _LIB_SPIRENT_PGA_COMMON_PRBS_H_

#include <cassert>
#include <cstdint>

namespace pga::prbs {

/*
 * These functions are for handling packet payloads with sequences from the
 * standard PRBS23 polynomial, x^23 + x^18 + 1.
 */

constexpr uint32_t shift_left(uint32_t value, unsigned count)
{
    assert(count <= 23);
    value &= 0x7fffff;
    return ((value << 23 | value) << count | value >> (23 - count));
}

constexpr uint32_t step(uint32_t seed)
{
    constexpr uint32_t masks[] = {
        0xffffffff,
        0xfffffe00,
        0x00003fff
    };

    return ((shift_left(seed, 9) & masks[0])
            ^ (shift_left(seed, 14) & masks[1])
            ^ (shift_left(seed, 19) & masks[2]));
}

}


#endif /* _LIB_SPIRENT_PGA_COMMON_PRBS_H_ */
