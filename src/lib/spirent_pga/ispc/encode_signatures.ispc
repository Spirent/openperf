#include "inet.h"
#include "signature_scramble.h"

export void encode_signatures(unsigned int8* uniform destinations[],
                              unsigned int32 const uniform stream_ids[],
                              unsigned int32 const uniform sequence_numbers[],
                              uniform unsigned int64 timestamp,
                              uniform unsigned int flags,
                              uniform unsigned int16 count)
{
    uniform int scratch[4 * programCount];

    /* We treat the signature as 4 x 32 bit integers */
    unsigned int<4> data;
    unsigned int<4> mask;
    unsigned int<4> signature;

    foreach (i = 0 ... count) {
        /*
         * Split the timestamp into 32 bit hi/lo values as it's
         * faster to operate on 32 bit values when using SIMD lanes.
         * Note: x = hi, y = lo
         */
        unsigned int<2> ts = { timestamp + i >> 32, timestamp + i };

        /* Shuffle everything into place */
        data[0] = stream_ids[i] >> 8;

        data[1] = stream_ids[i] << 24;
        data[1] |= ((sequence_numbers[i] >> 8) & 0xffff00) ^ 0xffff00;
        data[1] |= (sequence_numbers[i] >> 24);

        data[2] = sequence_numbers[i] << 8;
        data[2] |= ts.y >> 24;

        data[3] = ts.y << 8;
        data[3] |= (ts.x << 2) & 0xfc;
        data[3] |= flags & 0x3;

        /*
         * Signature is xor'ed with a mask based on the complement
         * of sequence byte 0; get the mask.
         */
        get_scramble_mask((sequence_numbers[i] & 0xff) ^ 0xff, mask);

        signature[0] = htonl(data[0] ^ mask[0]);
        signature[1] = htonl(data[1] ^ mask[1]);
        signature[2] = htonl(data[2] ^ mask[2]);
        signature[3] = htonl(data[3] ^ mask[3]);

        /* Write signature data out as an array of structs in our scratch area */
        soa_to_aos4((int)signature[0],
                    (int)signature[1],
                    (int)signature[2],
                    (int)signature[3],
                    scratch);

        /* And scatter the signatures into place */
        memcpy(destinations[i], &scratch[programIndex * 4], 16);
    }
}
