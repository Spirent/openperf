#include "arpa/inet.h"

#include "signature_scramble.h"

namespace scalar {

void encode_signatures(uint8_t* destinations[],
                       const uint32_t stream_ids[],
                       const uint32_t sequence_numbers[],
                       uint64_t timestamp,
                       int flags,
                       uint16_t count)
{
    std::array<uint32_t, 4> data;

    for (uint16_t i = 0; i < count; i++) {
        auto ts = timestamp + i;

        data[0] = stream_ids[i] >> 8;

        data[1] = stream_ids[i] << 24;
        data[1] |= ((sequence_numbers[i] >> 8) & 0xffff00) ^ 0xffff00;
        data[1] |= (sequence_numbers[i] >> 24);

        data[2] = sequence_numbers[i] << 8;
        data[2] |= (ts >> 24) & 0xff;

        data[3] = ts << 8;
        data[3] |= (ts >> 30) & 0xfc;
        data[3] |= flags & 0x3;

        /*
         * Signature is xor'ed with a mask based on the complement
         * of sequence byte 0; get the mask.
         */
        uint8_t scramble_key = (sequence_numbers[i] & 0xff);
        auto mask = pga::signature::get_scramble_mask(scramble_key ^ 0xff);

        auto signature = reinterpret_cast<uint32_t*>(destinations[i]);
        signature[0] = htonl(data[0] ^ mask[0]);
        signature[1] = htonl(data[1] ^ mask[1]);
        signature[2] = htonl(data[2] ^ mask[2]);
        signature[3] = htonl(data[3] ^ mask[3]);
    }
}

} // namespace scalar
