
inline static unsigned int rotate_left(unsigned int value, uniform unsigned int8 count)
{
    assert(count > 0 && count < 8);
    switch (count) {
    case 1: return (value << 1 | value >> 31);
    case 2: return (value << 2 | value >> 30);
    case 3: return (value << 3 | value >> 29);
    case 4: return (value << 4 | value >> 28);
    case 5: return (value << 5 | value >> 27);
    case 6: return (value << 6 | value >> 26);
    case 7: return (value << 7 | value >> 25);
    }
}

inline static unsigned int scamble(unsigned int byte,
                                         uniform unsigned int index)
{
    static const uniform int masks[][4] = {
        { 0xffff8404, 0x7807807b, 0x87807c7b, 0x838003ff },
        { 0x00f0f7f8, 0x0808fff7, 0x070ffff8, 0xf0f000f0 },
        { 0x00100e1f, 0xf01001ee, 0x01f1fe1f, 0xeff01ff0 },
        { 0x00203c1c, 0x3fdfc01c, 0x3fc003dc, 0x23c023c0 },
        { 0x003fb807, 0xb8384078, 0x47bf8047, 0xbfbfc03f },
        { 0x00f08fff, 0x8ff08070, 0x808f807f, 0x708fff0f },
        { 0x00e0fe1f, 0xffe0fe1e, 0xfefe01e0, 0xe0ffe0ff },
        { 0x003fc203, 0xc03fc1fd, 0xc1c1fdc3, 0xc3c03c3f }
    };

    unsigned int base = byte << 24 | byte << 16 | byte << 8 | byte;

    return ((base & masks[0][index])
            ^ (rotate_left(base, 1) & masks[1][index])
            ^ (rotate_left(base, 2) & masks[2][index])
            ^ (rotate_left(base, 3) & masks[3][index])
            ^ (rotate_left(base, 4) & masks[4][index])
            ^ (rotate_left(base, 5) & masks[5][index])
            ^ (rotate_left(base, 6) & masks[6][index])
            ^ (rotate_left(base, 7) & masks[7][index]));
}

inline static void get_scramble_mask(unsigned int8 seed, unsigned int<4>& mask)
{
    mask[0] = scamble(seed, 0);
    mask[1] = scamble(seed, 1);
    mask[2] = scamble(seed, 2);
    mask[3] = scamble(seed, 3);
}
