#include "signature_scramble.h"

namespace scalar {

void encode_signatures(uint8_t* destinations[],
                       const uint32_t stream_ids[],
                       const uint32_t sequence_numbers[],
                       uint64_t timestamp, int flags, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        auto signature = reinterpret_cast<uint32_t*>(destinations[i]);
        auto ts = timestamp + i;

        signature[0] = stream_ids[i] >> 8;

        signature[1] = stream_ids[i] << 24;
        signature[1] |= ((sequence_numbers[i] >> 8) & 0xffff00) ^ 0xffff00;
        signature[1] |= (sequence_numbers[i] >> 24);

        signature[2] = sequence_numbers[i] << 8;
        signature[2] |= (ts >> 24) & 0xff;

        signature[3] = ts << 8;
        signature[3] |= (ts >> 30) & 0xfc;
        signature[3] |= flags & 0x3;

        /*
         * Signature is xor'ed with a mask based on the complement
         * of sequence byte 0; get the mask.
         */
        auto mask = pga::signature::get_scramble_mask((sequence_numbers[i] & 0xff) ^ 0xff);
        signature[0] ^= mask[0];
        signature[1] ^= mask[1];
        signature[2] ^= mask[2];
        signature[3] ^= mask[3];
    }
}

}
