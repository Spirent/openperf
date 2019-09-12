#ifndef _LIB_SPIRENT_PGA_SCALAR_SIGNATURE_SCRAMBLE_H_
#define _LIB_SPIRENT_PGA_SCALAR_SIGNATURE_SCRAMBLE_H_

#include <array>
#include <cstdint>
#include <climits>

namespace pga::signature {

inline uint32_t rotate_left(uint32_t value, unsigned int count)
{
    constexpr unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count | value >> (-count & mask));
}

inline uint32_t scramble(uint8_t byte, unsigned index)
{
    static constexpr uint32_t masks[][4] = {
        { 0xffff8404, 0x7807807b, 0x87807c7b, 0x838003ff },
        { 0x00f0f7f8, 0x0808fff7, 0x070ffff8, 0xf0f000f0 },
        { 0x00100e1f, 0xf01001ee, 0x01f1fe1f, 0xeff01ff0 },
        { 0x00203c1c, 0x3fdfc01c, 0x3fc003dc, 0x23c023c0 },
        { 0x003fb807, 0xb8384078, 0x47bf8047, 0xbfbfc03f },
        { 0x00f08fff, 0x8ff08070, 0x808f807f, 0x708fff0f },
        { 0x00e0fe1f, 0xffe0fe1e, 0xfefe01e0, 0xe0ffe0ff },
        { 0x003fc203, 0xc03fc1fd, 0xc1c1fdc3, 0xc3c03c3f }
    };

    auto base = byte << 24 | byte << 16 | byte << 8 | byte;

    return ((base & masks[0][index])
            ^ (rotate_left(base, 1) & masks[1][index])
            ^ (rotate_left(base, 2) & masks[2][index])
            ^ (rotate_left(base, 3) & masks[3][index])
            ^ (rotate_left(base, 4) & masks[4][index])
            ^ (rotate_left(base, 5) & masks[5][index])
            ^ (rotate_left(base, 6) & masks[6][index])
            ^ (rotate_left(base, 7) & masks[7][index]));
}

inline std::array<uint32_t, 4> get_scramble_mask(uint8_t seed)
{
    return { scramble(seed, 0),
             scramble(seed, 1),
             scramble(seed, 2),
             scramble(seed, 3) };
}

}

#endif /* _LIB_SPIRENT_PGA_SCALAR_SIGNATURE_SCRAMBLE_H_ */
