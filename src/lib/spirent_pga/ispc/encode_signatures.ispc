#include "inet.h"
#include "signature_scramble.h"

#if TARGET_WIDTH == 4

/*
 * When using a width of four, we can easily optimize the signature copy
 * because the signature fits in one SIMD register. Here we use all lanes
 * to copy the scratch data into the destination array.
 */
inline void write_signatures(unsigned int32 s0,
                             unsigned int32 s1,
                             unsigned int32 s2,
                             unsigned int32 s3,
                             uniform unsigned int8* uniform signatures[])
{
    uniform int scratch[4 * programCount];

    /* Shuffle the signature data into a linear array */
    soa_to_aos4((int)s0,
                (int)s1,
                (int)s2,
                (int)s3,
                scratch);

    /* And efficiently copy each signature into place */
    foreach_active(index) {
        uniform int* uniform dst = (uniform int* uniform)signatures[index];
        unmasked {
            dst[programIndex] = scratch[4 * index + programIndex];
        }
    }
}

#else

inline void write_signatures(unsigned int32 s0,
                             unsigned int32 s1,
                             unsigned int32 s2,
                             unsigned int32 s3,
                             uniform unsigned int8* uniform signatures[])
{
    uniform int scratch[4 * programCount];

    /* Shuffle the signature data into a linear array */
    soa_to_aos4((int)s0,
                (int)s1,
                (int)s2,
                (int)s3,
                scratch);

    /* And copy each signature into place */
    foreach_active(index) {
        memcpy(signatures[index], &scratch[4 * index], 16);
    }
}

#endif

export void encode_signatures(unsigned int8* uniform destinations[],
                              unsigned int32 const uniform stream_ids[],
                              unsigned int32 const uniform sequence_numbers[],
                              unsigned int32 const uniform timestamps_hi[],
                              unsigned int32 const uniform timestamps_lo[],
                              unsigned int const uniform flags[],
                              uniform unsigned int16 count)
{
    /* We treat the signature as 4 x 32 bit integers */
    unsigned int<4> data;
    unsigned int<4> mask;
    unsigned int<4> signature;

    foreach (i = 0 ... count) {
        /* Shuffle everything into place */
        data[0] = stream_ids[i] >> 8;

        data[1] = stream_ids[i] << 24;
        data[1] |= ((sequence_numbers[i] >> 8) & 0xffff00) ^ 0xffff00;
        data[1] |= (sequence_numbers[i] >> 24);

        data[2] = sequence_numbers[i] << 8;
        data[2] |= timestamps_lo[i] >> 24;

        data[3] = timestamps_lo[i] << 8;
        data[3] |= (timestamps_hi[i] << 2) & 0xfc;
        data[3] |= flags[i] & 0x3;

        /*
         * Signature is xor'ed with a mask based on the complement
         * of sequence byte 0; get the mask.
         */
        get_scramble_mask((sequence_numbers[i] & 0xff) ^ 0xff, mask);

        signature[0] = htonl(data[0] ^ mask[0]);
        signature[1] = htonl(data[1] ^ mask[1]);
        signature[2] = htonl(data[2] ^ mask[2]);
        signature[3] = htonl(data[3] ^ mask[3]);

        /* Write signature data to destinations */
        write_signatures(signature[0],
                         signature[1],
                         signature[2],
                         signature[3],
                         &destinations[extract(i, 0)]);
    }
}
