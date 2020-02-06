#include "signature_scramble.h"

namespace scalar {

static constexpr int is_valid = (1 << 2);

uint16_t decode_signatures(const uint8_t* const payloads[],
                           uint16_t count,
                           uint32_t stream_ids[],
                           uint32_t sequence_numbers[],
                           uint32_t timestamps_lo[],
                           uint32_t timestamps_hi[],
                           int flags[])
{
    std::array<uint32_t, 4> signature;
    uint16_t nb_sigs = 0;

    for (uint16_t i = 0; i < count; i++) {
        auto data = reinterpret_cast<const uint32_t*>(payloads[i]);

        /* Unscramble the possible signature data */
        uint8_t scramble_key = ~(data[0] >> 24);
        auto mask = pga::signature::get_scramble_mask(scramble_key ^ 0xff);
        signature[0] = data[0] ^ mask[0];
        signature[1] = data[1] ^ mask[1];
        signature[2] = data[2] ^ mask[2];
        signature[3] = data[3] ^ mask[3];

        /* Verify that the scramble_key and byte 10 are complements */
        uint8_t byte10 = (signature[2] >> 8) & 0xff;
        if (scramble_key != byte10) {
            flags[i] = 0;
            continue;
        }

        /* Perform enhanced detection check */
        uint16_t edm1 = ~(signature[1] >> 8) & 0xffff;
        uint16_t edm2 = (signature[1] << 8 | signature[2] >> 24) & 0xffff;
        if (edm1 != edm2) {
            flags[i] = 0;
            continue;
        }

        /* Valid signature found; copy data out */
        stream_ids[i] = signature[0] << 8 | signature[1] >> 24;
        sequence_numbers[i] = signature[1] << 24 | signature[2] >> 8;
        timestamps_lo[i] = signature[2] << 24 | signature[3] >> 8;
        timestamps_hi[i] = (signature[3] >> 2) & 0x3f;
        flags[i] = is_valid | (signature[3] & 0x3);
        nb_sigs++;
    }

    return (nb_sigs);
}

} // namespace scalar
